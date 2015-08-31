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
	TASK_DOWNLOAD_PL = 0,
    TASK_DOWNLOAD_FILES,
	TASK_STOP
} TaskId;

typedef enum {
	STATE_DOWNLOAD_PL = 0,
	STATE_DOWNLOAD_FILES,
	STATE_AVAILABLE
} StateId;

typedef enum {
    CLIENT_OK = 0,
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
	StateId stateId;
};	// g_ctx;

typedef struct NativeContext NativeContext;

static void task_download(NativeContext *);
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

static Task* get_task (NativeContext *g_ctx)
{
	Task *task = TAILQ_FIRST(&g_ctx->tasks);
	TAILQ_REMOVE(&g_ctx->tasks, task, next);
	return task;
}

static void put_task (Task *task, NativeContext *g_ctx)
{
	LOGE("*");
	pthread_mutex_lock(&g_ctx->mutex);
	LOGE("**");
	LOGI("Put task: %d\n", task->task);
	assert(task);
	TAILQ_INSERT_TAIL(&g_ctx->tasks, task, next);
	pthread_cond_broadcast(&g_ctx->cv);
	pthread_mutex_unlock(&g_ctx->mutex);
}

static void clear_tasks (NativeContext *g_ctx)
{
	while (TAILQ_FIRST(&g_ctx->tasks)) {
		Task *task = get_task(g_ctx);
		destroy_task(task);
	}
}

static void my_complete (Downloader *d, void *args, int status, size_t number_files_in_stack)
{
	NativeContext *g_ctx = (NativeContext*)((intptr_t)args);
	switch (g_ctx->stateId){
		case STATE_DOWNLOAD_PL:{
			if (status == -1){
				Task *task = create_task(TASK_STOP, (void*)args);
				if (!task){
					task = calloc(1, sizeof(Task));
					task->task = TASK_STOP;
					return;
				}
				put_task(task, g_ctx);
				break;
			}
			Task *task = create_task(TASK_DOWNLOAD_FILES, (void*)args);
			if (!task){
				task = calloc(1, sizeof(Task));
				task->task = TASK_DOWNLOAD_FILES;
				return;
			}
			put_task(task, g_ctx);
			break;
		}
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

static void task_download_playlist (NativeContext *g_ctx)
{
	if (g_ctx->stateId != STATE_AVAILABLE){
		return;
	}
	g_ctx->stateId = STATE_DOWNLOAD_PL;
	const char *url = "http://192.168.4.102:80/test2.txt";
	const char *name = "/sdcard/file.json";

	int res = downloader_add(g_ctx->d, url, name);
	if (res) {
		nativeDeinit(NULL, NULL, (jlong)((intptr_t)g_ctx));
	}
	LOGI("Playlist downloaded\n");
}

static jlong nativeInit (JNIEnv *env, jobject obj)
{
	NativeContext *g_ctx = calloc(1, sizeof(NativeContext));

	if(!g_ctx){
		return 0;
	}

	assert(g_ctx);

	g_ctx->d = NULL;
	g_ctx->playlist = NULL;
	g_ctx->shutdown = 0;
	g_ctx->thread_initialized = 0;
	g_ctx->mutex_initialized = 0;
	g_ctx->cv_initialized = 0;
	g_ctx->stateId = STATE_AVAILABLE;
	g_ctx->d = downloader_create(&my_callbacks, (void*)g_ctx);
	if (!g_ctx->d){
		return 0;
	}
	int timeout = 1000 * 2;

	downloader_set_timeout_connection(g_ctx->d, timeout);
	downloader_set_timeout_recieve(g_ctx->d, timeout);

	int task_mutex_error = pthread_mutex_init(&g_ctx->mutex, NULL);
	int task_cv_error = pthread_cond_init(&g_ctx->cv, NULL);

	if (task_mutex_error || task_cv_error){
		goto exit;
	}

	g_ctx->mutex_initialized = 1;
	g_ctx->cv_initialized = 1;

	TAILQ_INIT(&g_ctx->tasks);

	if (pthread_create(&g_ctx->task_thread, NULL, (void*)task_flow, (void*)g_ctx)) {
		goto exit;
	}
	return CLIENT_OK;
exit:
	if (g_ctx->mutex_initialized) {
		pthread_mutex_destroy(&g_ctx->mutex);
	}
	if (g_ctx->cv_initialized) {
		pthread_cond_destroy(&g_ctx->cv);
	}
	return 0;
}

static void nativeDeinit(JNIEnv *env, jobject obj, jlong args)
{
	NativeContext *g_ctx = (NativeContext*)((intptr_t)args);
	/*assert(g_ctx);

	pthread_mutex_lock(&g_ctx->mutex);
	LOGI("***");
	g_ctx->shutdown = 1;
	pthread_cond_broadcast(&g_ctx->cv);
	pthread_mutex_unlock(&g_ctx->mutex);
	pthread_join(g_ctx->task_thread, NULL);
	clear_tasks(g_ctx);

	playlist_destroy(g_ctx->playlist);
	downloader_destroy(g_ctx->d);

	if (g_ctx->cv_initialized) {
		pthread_cond_destroy(&g_ctx->cv);
	}

	if (g_ctx->mutex_initialized) {
		pthread_mutex_destroy(&g_ctx->mutex);
	}
*/
	LOGI("finished\n");
}

static void task_stop (NativeContext *g_ctx)
{
	if (g_ctx->stateId == STATE_AVAILABLE) {
		return;
	}

	downloader_stop(g_ctx->d);
	g_ctx->stateId = STATE_AVAILABLE;
	nativeDeinit(NULL, NULL, (jlong)((intptr_t)g_ctx));
	nativeInit(NULL, NULL);
}

static int parse_playlist (NativeContext *g_ctx)
{
	g_ctx->playlist = playlist_create();
	if (!g_ctx->playlist) {
		return CLIENT_ERROR;
	}

	const char *name = "/sdcard/file.json";
	int parse_res = playlist_parse(g_ctx->playlist, name);
	if (!parse_res) {
		LOGE("Error parse\n");
		return CLIENT_ERROR;
	}
	return CLIENT_OK;
}

static int download_files (NativeContext *g_ctx)
{
	assert(g_ctx);
	for (int i = 0; i < g_ctx->playlist->items_count && !g_ctx->shutdown; i++) {
		char *path = "/sdcard/";
		size_t length = strlen(g_ctx->playlist->items[i].name) + strlen(path);
		char *new_name = malloc(length + 1);

		if (!new_name){
			return CLIENT_ERROR;
		}

		snprintf(new_name, length + 1, "%s%s", path, g_ctx->playlist->items[i].name);
		free(g_ctx->playlist->items[i].name);
		g_ctx->playlist->items[i].name = new_name;
		downloader_add(g_ctx->d, g_ctx->playlist->items[i].uri, g_ctx->playlist->items[i].name);
	}
	return CLIENT_OK;
}

void task_download(NativeContext *g_ctx)
{
	if (g_ctx->stateId == STATE_DOWNLOAD_FILES){
		return;
	}
	g_ctx->stateId = STATE_DOWNLOAD_FILES;

	int res = parse_playlist(g_ctx);
	if(res == CLIENT_ERROR){
		nativeDeinit(NULL, NULL, (jlong)((intptr_t)g_ctx));
		return;
	}

	res = download_files(g_ctx);
	if(res == CLIENT_ERROR){
		nativeDeinit(NULL, NULL, (jlong)((intptr_t)g_ctx));
		return;
	}
}

static void* task_flow (void *args)
{
	NativeContext *g_ctx = (NativeContext*) args;
	assert(g_ctx);

	while(1)
	{
		pthread_mutex_lock(&g_ctx->mutex);
		while (TAILQ_EMPTY(&g_ctx->tasks) && !g_ctx->shutdown) {
			LOGE("*** wait ***");
			pthread_cond_wait(&g_ctx->cv, &g_ctx->mutex);
		}

		if (g_ctx->shutdown) {
			pthread_mutex_unlock(&g_ctx->mutex);
			break;
		}

		Task *current_task = get_task(g_ctx);
		pthread_mutex_unlock(&g_ctx->mutex);
		switch(current_task->task){
			case TASK_DOWNLOAD_PL:{
				task_download_playlist(g_ctx);
				LOGE("TASK_DOWNLOAD_PL");
				break;
			}
			case TASK_DOWNLOAD_FILES:{
				task_download(g_ctx);
				LOGE("TASK_DOWNLOAD_FILES");
				break;
			}
			case TASK_STOP:{
				task_stop(g_ctx);
				LOGE("TASK STOP");
				break;
			}
		}
		destroy_task(current_task);
	}
	return NULL;
}

static void nativeStartDownloading (JNIEnv *env, jobject obj, jlong args)
{
	NativeContext *g_ctx = (NativeContext*)((intptr_t)args);
	Task *task = create_task(TASK_DOWNLOAD_PL, (void*)args);
	if (!task) {
		task->task = TASK_DOWNLOAD_PL;
		return;
	}
	put_task(task, g_ctx);
}

static void nativeStopDownloading (JNIEnv *env, jobject obj, jlong args)
{
	NativeContext *g_ctx = (NativeContext*)((intptr_t)args);
	Task *task = create_task(TASK_STOP, NULL);
	if (!task){
		task->task = TASK_STOP;
		return;
	}
	put_task(task, g_ctx);
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
