#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <downloader/downloader.h>
#include <parser/parser.h>
#include <http_client.h>
#include <sys/queue.h>

#define LOG_TAG	"test"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)

typedef enum {
	TASK_DOWNLOAD_PL,
    TASK_DOWNLOAD_FILES,
	TASK_STOP
} TaskId;

typedef enum {
	STATE_DOWNLOAD_PL,
	STATE_DOWNLOAD_FILES,
	STATE_AVAILABLE
} StateId;

typedef enum {
    CLIENT_OK,
	CLIENT_ERROR
} ClientStatus;

typedef struct Task
{
	TAILQ_ENTRY(Task) next;
	TaskId task;
} Task;

typedef TAILQ_HEAD(Tasks, Task) Tasks;

struct NativeContext
{
	Downloader *d;
	Playlist *playlist;

	pthread_mutex_t mutex;
	pthread_cond_t cv;
	pthread_t task_thread;
	bool thread_initialized;
	bool mutex_initialized;
	bool cv_initialized;
	int shutdown;
	Tasks tasks;
	StateId state_id;
};

typedef struct NativeContext NativeContext;

static void task_stop (NativeContext *context);
static void task_download_files(NativeContext *);
static void *task_flow(void *args);
static void nativeDeinit(JNIEnv *env, jobject obj, jlong args);
static jlong nativeInit();

static void destroy_task(Task *task)
{
	free(task);
}
static Task* create_task(TaskId taskId, void *args)
{
	Task *task = calloc(1,sizeof(Task));
	if (task){
		task->task = taskId;
	}
	return task;
}

static Task* get_task (NativeContext *context)
{
	Task *task = TAILQ_FIRST(&context->tasks);
	TAILQ_REMOVE(&context->tasks, task, next);
	return task;
}

static void put_task (Task *task, NativeContext *context)
{
	pthread_mutex_lock(&context->mutex);
	LOGI("Put task: %d\n", task->task);
	assert(task);
	TAILQ_INSERT_TAIL(&context->tasks, task, next);
	pthread_cond_broadcast(&context->cv);
	pthread_mutex_unlock(&context->mutex);
}

static void clear_tasks (NativeContext *context)
{
	while (TAILQ_FIRST(&context->tasks)) {
		Task *task = get_task(context);
		destroy_task(task);
	}
}

static void my_complete (Downloader *d, void *args, int status, size_t number_files_in_stack)
{
	NativeContext *context = (NativeContext*)((intptr_t)args);
	switch (context->state_id){
		case STATE_DOWNLOAD_PL:
			if (status == -1){
				Task *task = create_task(TASK_STOP, (void*)args);
				if (!task){
					task = calloc(1, sizeof(Task));
					task->task = TASK_STOP;
					return;
				}
				put_task(task, context);
				break;
			}
			Task *task = create_task(TASK_DOWNLOAD_FILES, (void*)args);
			if (!task){
				task = calloc(1, sizeof(Task));
				task->task = TASK_DOWNLOAD_FILES;
				return;
			}
			put_task(task, context);
			break;
	}
	LOGI("Downloaded");
}

static void my_progress (Downloader *d, void *args, int64_t curr_size, int64_t total_size)
{
	if (total_size == 0) {
		LOGI("%d", curr_size);
	} else {
		int currPercent = (curr_size * 100) / total_size;
		LOGI("%d %%", currPercent);
	}
	return;
}

static const IDownloader_Cb my_callbacks =
{
		.complete = &my_complete,
		.progress = &my_progress
};

static void task_download_playlist (NativeContext *context)
{
	const char *url = "http://192.168.4.102:80/test2.txt";
	const char *name = "/sdcard/file.json";

	int res = downloader_add(context->d, url, name);
	if (res) {
		task_stop(context);
	}
	LOGI("Playlist downloaded\n");
}

static jlong nativeInit (JNIEnv *env, jobject obj)
{
	NativeContext *context = calloc(1, sizeof(NativeContext));

	if(!context){
		return 0;
	}

	context->d = NULL;
	context->playlist = NULL;
	context->shutdown = 0;
	context->thread_initialized = 0;
	context->mutex_initialized = 0;
	context->cv_initialized = 0;
	context->state_id = STATE_AVAILABLE;
	context->d = downloader_create(&my_callbacks, (void*)context);

	if (!context->d){
		free(context);
		return 0;
	}
	int timeout = 1000 * 2;

	downloader_set_timeout_connection(context->d, timeout);
	downloader_set_timeout_recieve(context->d, timeout);

	int task_mutex_error = pthread_mutex_init(&context->mutex, NULL);
	int task_cv_error = pthread_cond_init(&context->cv, NULL);

	if (task_mutex_error || task_cv_error){
		goto exit;
	}

	context->mutex_initialized = 1;
	context->cv_initialized = 1;

	TAILQ_INIT(&context->tasks);

	if (pthread_create(&context->task_thread, NULL, (void*)task_flow, (void*)context)) {
		goto exit;
	}
	return (jlong)((intptr_t)context);
exit:
	if (context->mutex_initialized) {
		pthread_mutex_destroy(&context->mutex);
	}
	if (context->cv_initialized) {
		pthread_cond_destroy(&context->cv);
	}
	if (context->d){
		downloader_destroy(context->d);
	}
	if (context->playlist){
		playlist_destroy(context->playlist);
	}
	free(context);
	return 0;
}

static void nativeDeinit(JNIEnv *env, jobject obj, jlong args)
{
	NativeContext *context = (NativeContext*)((intptr_t)args);
	assert(context);

	pthread_mutex_lock(&context->mutex);
	context->shutdown = 1;
	pthread_cond_broadcast(&context->cv);
	pthread_mutex_unlock(&context->mutex);
	pthread_join(context->task_thread, NULL);
	clear_tasks(context);

	playlist_destroy(context->playlist);
	downloader_destroy(context->d);

	if (context->cv_initialized) {
		pthread_cond_destroy(&context->cv);
	}

	if (context->mutex_initialized) {
		pthread_mutex_destroy(&context->mutex);
	}
	free(context);
	LOGI("finished\n");
}

static void task_stop (NativeContext *context)
{
	downloader_stop(context->d);
}

static int parse_playlist (NativeContext *context)
{
	context->playlist = playlist_create();
	if (!context->playlist) {
		return CLIENT_ERROR;
	}

	const char *name = "/sdcard/file.json";
	int parse_res = playlist_parse(context->playlist, name);
	if (!parse_res) {
		LOGE("Error parse\n");
		return CLIENT_ERROR;
	}
	return CLIENT_OK;
}

static int download_files (NativeContext *context)
{
	assert(context);
	for (int i = 0; i < context->playlist->items_count && !context->shutdown; i++) {
		char *path = "/sdcard/";
		size_t length = strlen(context->playlist->items[i].name) + strlen(path);
		char *new_name = malloc(length + 1);

		if (!new_name){
			return CLIENT_ERROR;
		}

		snprintf(new_name, length + 1, "%s%s", path, context->playlist->items[i].name);
		free(context->playlist->items[i].name);
		context->playlist->items[i].name = new_name;
		downloader_add(context->d, context->playlist->items[i].uri, context->playlist->items[i].name);
	}
	return CLIENT_OK;
}

void task_download_files(NativeContext *context)
{
	int res = parse_playlist(context);
	if(res == CLIENT_ERROR){
		task_stop(context);
		return;
	}

	res = download_files(context);
	if(res == CLIENT_ERROR){
		task_stop(context);
		return;
	}
}

static void* task_flow (void *args)
{
	NativeContext *context = (NativeContext*) args;
	assert(context);

	while(1)
	{
		pthread_mutex_lock(&context->mutex);
		while (TAILQ_EMPTY(&context->tasks) && !context->shutdown) {
			pthread_cond_wait(&context->cv, &context->mutex);
		}

		if (context->shutdown) {
			pthread_mutex_unlock(&context->mutex);
			break;
		}

		Task *current_task = get_task(context);
		pthread_mutex_unlock(&context->mutex);
		switch(current_task->task){
			case TASK_DOWNLOAD_PL:
				if (context->state_id != STATE_AVAILABLE){
					break;
				}
				context->state_id = STATE_DOWNLOAD_PL;
				task_download_playlist(context);
				break;
			case TASK_DOWNLOAD_FILES:
				if (context->state_id == STATE_DOWNLOAD_FILES){
					break;
				}
				context->state_id = STATE_DOWNLOAD_FILES;
				task_download_files(context);
				break;
			case TASK_STOP:
				if (context->state_id == STATE_AVAILABLE) {
					break;
				}
				context->state_id = STATE_AVAILABLE;
				task_stop(context);
				break;
		}
		destroy_task(current_task);
	}
	return NULL;
}

static void nativeStartDownloading (JNIEnv *env, jobject obj, jlong args)
{
	NativeContext *context = (NativeContext*)((intptr_t)args);
	assert(context);
	Task *task = create_task(TASK_DOWNLOAD_PL, (void*)args);
	if (!task) {
		return;
	}
	put_task(task, context);
}

static void nativeStopDownloading (JNIEnv *env, jobject obj, jlong args)
{
	NativeContext *context = (NativeContext*)((intptr_t)args);
	assert(context);
	Task *task = create_task(TASK_STOP, NULL);
	if (!task){
		return;
	}
	put_task(task, context);
}

static JNINativeMethod methodTable[] =
{
	{"nativeStartDownloading", "(J)V", (void *)nativeStartDownloading},
	{"nativeStopDownloading",  "(J)V", (void *)nativeStopDownloading},
	{"nativeInit",  "()J", (void *)nativeInit},
	{"nativeDeinit",  "(J)V", (void *)nativeDeinit}
};

jint JNI_OnLoad (JavaVM *vm, void *reserved)
{
	if (!vm) {
		return JNI_ERR;
	}

	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}

	if (http_client_on_load(vm) != JNI_OK) {
		return JNI_ERR;
	}

	jclass class = (*env)->FindClass(env,"com/example/testandroidapp/MainActivity");
	if (!class){
		return JNI_ERR;
	}

	(*env)->RegisterNatives(env, class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );

	return JNI_VERSION_1_6;
}
