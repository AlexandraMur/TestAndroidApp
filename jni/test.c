#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <downloader/downloader.h>
#include <parser/parser.h>

#define LOG_TAG	"test"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)

struct NativeContext
{
	Downloader *d;
} g_ctx;

typedef struct
{
	int num;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	int mutex_flag;
	int cv_flag;
} Semaphore;

static pthread_t g_thread;

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

static void* workFlow (void* arg_)
{
	long arg = (long) arg_;
	IDownloader_Cb my_callbacks =
	{
		.complete = &my_complete,
		.progress = &my_progress
	};

	g_ctx.d = NULL;
	Playlist *playlist = NULL;

	Semaphore sync = {0};

	int mutex_error = pthread_mutex_init(&sync.mutex, NULL);
	int cv_error = pthread_cond_init(&sync.cv, NULL);

	if (mutex_error || cv_error){
		goto exit;
	}

	sync.mutex_flag = 1;
	sync.cv_flag = 1;

	g_ctx.d = downloader_create(&my_callbacks, &sync);
	if (!g_ctx.d) {
		goto exit;
	}

	int timeout = 1000 * 2;

	downloader_set_timeout(g_ctx.d, timeout);

	//const char *url = "http://public.tv/api/?s=9c1997663576a8b11d1c4f8becd57e52&c=playlist_full&date=2015-07-06";
	const char *url = "http://192.168.4.102:80/test2.txt";
	const char *name = "/sdcard/file.json";

	pthread_mutex_lock(&sync.mutex);
	sync.num++;
	pthread_mutex_unlock(&sync.mutex);

	int res = downloader_add(g_ctx.d, url, name);
	if (res) {
		goto exit;
	}
	semaphore_wait(&sync);
	LOGI("Playlist downloaded\n");

	playlist = playlist_create();
	if (!playlist) {
		goto exit;
	}

	int parse_res = playlist_parse(playlist, name);
	if (!parse_res) {
		LOGE("Error parse\n");
	}
	for (int i = 0; i < playlist->items_count; i++) {
		semaphore_inc(&sync);
		char *path = "/sdcard/";
		size_t length = strlen(playlist->items[i].name) + strlen(path);
		char *new_name = malloc(length + 1);
		if (!new_name){
			goto exit;
		}
		snprintf(new_name, length + 1, "%s%s", path, playlist->items[i].name);
		free(playlist->items[i].name);
		playlist->items[i].name = new_name;
		downloader_add(g_ctx.d, playlist->items[i].uri, playlist->items[i].name);
	}
	semaphore_wait(&sync);

exit:
	playlist_destroy(playlist);
	downloader_destroy(g_ctx.d);

	if (sync.mutex_flag) {
		pthread_mutex_destroy(&sync.mutex);
	}
	if (sync.cv_flag) {
		pthread_cond_destroy(&sync.cv);
	}
	LOGI("finished\n");
	return NULL;
}

static void startDownloading (jlong args)
{
	int result = pthread_create(&g_thread, NULL, (void*)workFlow, (void*)args);
	assert(result == 1);
}

static void stopDownloading ()
{
	downloader_stop(g_ctx.d);
}

static JNINativeMethod methodTable[] =
{
	{"startDownloading", "()V", (void *)startDownloading},
	{"stopDownloading",  "()V", (void *)stopDownloading}
};

jint JNI_OnLoad (JavaVM *vm, void *reserved)
{
	if (!vm) {
		return JNI_ERR;
	}
	if (downloader_OnLoad(vm) != JNI_OK) {
		return JNI_ERR;
	}
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}
	jclass class = (*env)->FindClass(env,"com/example/testandroidapp/MainActivity");
	if (!class){
		return JNI_ERR;
	}
	(*env)->RegisterNatives(env, class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );
	return JNI_VERSION_1_6;
}
