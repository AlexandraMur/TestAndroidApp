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
    TASK_PARSE_PL,
	TASK_STOP
} TaskId;

typedef enum {
	STATE_DOWNLOAD_PL,
	STATE_DOWNLOAD_FILES,
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

typedef struct
{
	int num;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	int mutex_flag;
	int cv_flag;
} Semaphore;

struct NativeContext
{
	Downloader *d;
	Semaphore sync;
	Playlist *playlist;

	pthread_mutex_t mutex;
	pthread_cond_t cv;
	pthread_t task_thread;
	pthread_t download_thread;
	bool thread_initialized;
	bool mutex_initialized;
	bool cv_initialized;
	int shutdown;
	Tasks tasks;
} g_ctx;

static void semaphore_wait (Semaphore *sync)
{
	pthread_mutex_lock(&sync->mutex);
	while(sync->num){
		pthread_cond_wait(&sync->cv, &sync->mutex);
	}
	pthread_mutex_unlock(&sync->mutex);
}

static void semaphore_inc (Semaphore *sync)
{
	pthread_mutex_lock(&sync->mutex);
	sync->num++;
	pthread_mutex_unlock(&sync->mutex);
}

static void semaphore_dec (Semaphore *sync)
{
	pthread_mutex_lock(&sync->mutex);
	sync->num--;
	pthread_cond_signal(&sync->cv);
	pthread_mutex_unlock(&sync->mutex);
}

static void my_complete (Downloader *d, void *args, int status, size_t number_files_in_stack)
{
	LOGI("Downloaded");
	semaphore_dec((Semaphore*)args);
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

static Task* get_task ()
{
	Task *task = TAILQ_FIRST(&g_ctx.tasks);
	TAILQ_REMOVE(&g_ctx.tasks, task, next);
	return task;
}

static void clear_tasks ()
{
	while (TAILQ_FIRST(&g_ctx.tasks)) {
		Task *task = get_task();
		free(task);
	}
}

static void nativeDeinit()
{
	playlist_destroy(g_ctx.playlist);
	downloader_destroy(g_ctx.d);

	if (g_ctx.sync.mutex_flag) {
		pthread_mutex_destroy(&g_ctx.sync.mutex);
	}
	if (g_ctx.sync.cv_flag) {
		pthread_cond_destroy(&g_ctx.sync.cv);
	}

	pthread_mutex_lock(&g_ctx.mutex);
	g_ctx.shutdown = 1;
	pthread_cond_broadcast(&g_ctx.cv);
	pthread_mutex_unlock(&g_ctx.mutex);
	pthread_join(g_ctx.task_thread, NULL);
	pthread_join(g_ctx.download_thread, NULL);
	clear_tasks();

	if (g_ctx.cv_initialized) {
		pthread_cond_destroy(&g_ctx.cv);
	}

	if (g_ctx.mutex_initialized) {
		pthread_mutex_destroy(&g_ctx.mutex);
	}

	LOGI("finished\n");
}

static void* download (void* arg_)
{
	long arg = (long) arg_;
	//const char *url = "http://public.tv/api/?s=9c1997663576a8b11d1c4f8becd57e52&c=playlist_full&date=2015-07-06";
	const char *url = "http://192.168.4.102:80/test2.txt";
	const char *name = "/sdcard/file.json";

	pthread_mutex_lock(&g_ctx.sync.mutex);
	g_ctx.sync.num++;
	pthread_mutex_unlock(&g_ctx.sync.mutex);

	int res = downloader_add(g_ctx.d, url, name);
	if (res) {
		nativeDeinit();
		return NULL;
	}
	semaphore_wait(&g_ctx.sync);
	LOGI("Playlist downloaded\n");

	g_ctx.playlist = playlist_create();
	if (!g_ctx.playlist) {
		nativeDeinit();
		return NULL;
	}

	int parse_res = playlist_parse(g_ctx.playlist, name);
	if (!parse_res) {
		LOGE("Error parse\n");
	}
	for (int i = 0; i < g_ctx.playlist->items_count && !g_ctx.shutdown; i++) {
		semaphore_inc(&g_ctx.sync);
		char *path = "/sdcard/";
		size_t length = strlen(g_ctx.playlist->items[i].name) + strlen(path);
		char *new_name = malloc(length + 1);

		if (!new_name){
			nativeDeinit();
			return NULL;
		}

		snprintf(new_name, length + 1, "%s%s", path, g_ctx.playlist->items[i].name);
		free(g_ctx.playlist->items[i].name);
		g_ctx.playlist->items[i].name = new_name;
		downloader_add(g_ctx.d, g_ctx.playlist->items[i].uri, g_ctx.playlist->items[i].name);
	}
	return NULL;
}

static void startDownloading(void *args)
{
	LOGI("StartDownloading thread");
	download((void*)args);
}

static void stopDownloading()
{
	LOGI("StopDownloading");
	downloader_stop(g_ctx.d);
}

static void* taskFlow (void *args)
{
	while(1)
	{
		pthread_mutex_lock(&g_ctx.mutex);
		while (TAILQ_EMPTY(&g_ctx.tasks) && !g_ctx.shutdown){
			pthread_cond_wait(&g_ctx.cv, &g_ctx.mutex);
		}
		if (g_ctx.shutdown) {
			pthread_mutex_unlock(&g_ctx.mutex);
			break;
		}
		Task *current_task = get_task();
		pthread_mutex_unlock(&g_ctx.mutex);

		switch(current_task->task){
			case TASK_DOWNLOAD_PL:
				startDownloading(args);
				break;
			case TASK_PARSE_PL:
				startDownloading(args);
				break;
			case TASK_STOP:
				stopDownloading();
				break;
		}
	}
	return NULL;
}

static void put_task (Task *task)
{
	pthread_mutex_lock(&g_ctx.mutex);
	LOGI("Put task: %d\n", task->task);
	assert(task);
	TAILQ_INSERT_TAIL(&g_ctx.tasks, task, next);
	pthread_cond_broadcast(&g_ctx.cv);
	LOGI("put task");
	pthread_mutex_unlock(&g_ctx.mutex);
}

static int addTask (TaskId taskId, void *args)
{
	Task *task = calloc(1,sizeof(Task));
	task->task = taskId;
	put_task(task);
	return CLIENT_OK;
}

static int initDownloader ()
{
	g_ctx.d = NULL;
	g_ctx.playlist = NULL;
	g_ctx.sync.num = 0;
	g_ctx.sync.mutex_flag = 0;
	g_ctx.sync.cv_flag = 0;
	g_ctx.shutdown = 0;
	g_ctx.thread_initialized = 0;
	g_ctx.mutex_initialized = 0;
	g_ctx.cv_initialized = 0;

	int sync_mutex_error = pthread_mutex_init(&g_ctx.sync.mutex, NULL);
	int sync_cv_error = pthread_cond_init(&g_ctx.sync.cv, NULL);

	if (sync_mutex_error || sync_cv_error){
		goto exit;
	}

	g_ctx.sync.mutex_flag = 1;
	g_ctx.sync.cv_flag = 1;

	g_ctx.d = downloader_create(&my_callbacks, &g_ctx.sync);
	if (!g_ctx.d) {
		goto exit;
	}

	int timeout = 1000 * 2;

	downloader_set_timeout_connection(g_ctx.d, timeout);
	downloader_set_timeout_recieve(g_ctx.d, timeout);

	return CLIENT_OK;
exit:
	nativeDeinit();
	return CLIENT_ERROR;
}

static int nativeInit ()
{
	if (initDownloader()) {
		return CLIENT_ERROR;
	}

	int task_mutex_error = pthread_mutex_init(&g_ctx.mutex, NULL);
	int task_cv_error = pthread_cond_init(&g_ctx.cv, NULL);

	if (task_mutex_error || task_cv_error){
		goto exit;
	}

	g_ctx.mutex_initialized = 1;
	g_ctx.cv_initialized = 1;

	TAILQ_INIT(&g_ctx.tasks);

	if (pthread_create(&g_ctx.task_thread, NULL, (void*)taskFlow, (void*)g_ctx.d)) {
		goto exit;
	}
	return CLIENT_OK;
exit:
	if (g_ctx.mutex_initialized) {
		pthread_mutex_destroy(&g_ctx.mutex);
	}
	if (g_ctx.cv_initialized) {
		pthread_cond_destroy(&g_ctx.cv);
	}
	return CLIENT_ERROR;
}

static void nativeStartDownloading (jlong args)
{
	addTask(TASK_PARSE_PL, NULL);
}

static void nativeStopDownloading ()
{
	addTask(TASK_STOP, NULL);
}

static JNINativeMethod methodTable[] =
{
	{"nativeStartDownloading", "()V", (void *)nativeStartDownloading},
	{"nativeStopDownloading",  "()V", (void *)nativeStopDownloading},
	{"nativeInit",  "()I", (void *)nativeInit},
	{"nativeDeinit",  "()V", (void *)nativeDeinit}
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
