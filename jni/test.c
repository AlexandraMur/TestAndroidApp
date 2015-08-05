#include "com_example_testandroidapp_MainActivity.h"
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <downloader/downloader.h>
#include <parser/parser.h>

struct syncronize {
	int num;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	int mutex_flag;
	int cv_flag;
};

static void my_progress(Downloader *d, void *args, char *url, char *name_of_file, int64_t curr_size, int64_t total_size){
	int64_t curr_percent = (curr_size * 100) / total_size;
	printf("DOWN %s: %" PRId64 " of 100%%\n", name_of_file, curr_percent);
}

static void my_complete(Downloader *d, void *args, char *url, char *name_of_file, int status, size_t number_files_in_stack){
	struct syncronize *sync = (struct syncronize*) args;
	if (status == DOWNLOADER_STATUS_ERROR){
		printf("ERROR %s\n", name_of_file);
	}

	if(status == DOWNLOADER_STATUS_CONT){
		printf("File wasn't modified\n");
	}

	if (status == DOWNLOADER_STATUS_OK){
		printf("DOWNLOADED %s\n", name_of_file);
	}
	if (status == DOWNLOADER_STATUS_EXIT){
		printf("EXIT\n");
	}

	pthread_mutex_lock(&sync->mutex);
	sync->num--;
	pthread_cond_broadcast(&sync->cv);
	pthread_mutex_unlock(&sync->mutex);

	printf ("%zu file/s in stack\n", number_files_in_stack);
}

/*
static void downloadThroughtJava(){
	JNIEnv *pEnv = NULL;
	if ((*globalVm)->AttachCurrentThread(globalVm, &pEnv, NULL) != JNI_OK){
		goto exit;
	}
	jstring jStr = (*pEnv)->NewStringUTF(pEnv, "http://idev.by/android/22971/");
	long args = NULL;
	(*pEnv)->CallVoidMethod(pEnv, globalMyDownloaderObj, globalDownloadID, jStr, args);
exit:
	(*globalVm)->DetachCurrentThread(globalVm);
}
*/
static void workFlow (){
	IDownloader_Cb my_callbacks;
	my_callbacks.complete = &my_complete;
	my_callbacks.progress = &my_progress;

	Downloader *d = NULL;
	Playlist *playlist = NULL;

	struct syncronize sync;
	sync.mutex_flag = 0;
	sync.cv_flag = 0;
	sync.num = 0;

	int mutex_error = pthread_mutex_init(&sync.mutex, NULL);
	int cv_error = pthread_cond_init(&sync.cv, NULL);

	if (mutex_error || cv_error){
		goto exit;
	}

	sync.mutex_flag = 1;
	sync.cv_flag = 1;

	d = downloader_create(&my_callbacks, &sync);
	if (!d){
		goto exit;
	}

	char *url = "http://localhost/test2.txt";
	char *name = "test2.txt";

	pthread_mutex_lock(&sync.mutex);
	sync.num++;
	pthread_mutex_unlock(&sync.mutex);

	int res = downloader_add(d, url, name);
	if (res){
		goto exit;
	}

	pthread_mutex_lock(&sync.mutex);
	while(sync.num){
		pthread_cond_wait(&sync.cv, &sync.mutex);
	}
	pthread_mutex_unlock(&sync.mutex);

	playlist = playlist_create();
	if (!playlist){
		goto exit;
	}

	playlist_parse(playlist, name);
	for(int i = 0; i < playlist->items_count; i++){
		pthread_mutex_lock(&sync.mutex);
		sync.num++;
		pthread_mutex_unlock(&sync.mutex);
		downloader_add(d, playlist->items[i].uri, playlist->items[i].name);
	}

	pthread_mutex_lock(&sync.mutex);
	while(sync.num){
		pthread_cond_wait(&sync.cv, &sync.mutex);
	}
	pthread_mutex_unlock(&sync.mutex);

exit:
	playlist_destroy(playlist);
	downloader_destroy(d);

	if (sync.mutex_flag){
		pthread_mutex_destroy(&sync.mutex);
	}
	if (sync.cv_flag){
		pthread_cond_destroy(&sync.cv);
	}
	printf("finished\n");
}

static void nativeTest (){
	pthread_t thread;
	pthread_create(&thread, NULL, (void*)workFlow, NULL);
}

static JNINativeMethod methodTable[] = {
	{"nativeTest", "()V", (void *)nativeTest}
};

jint JNI_OnLoad (JavaVM *vm, void *reserved){
	if (!vm){
		return JNI_ERR;
	}

	int downl_onload = downloader_OnLoad(vm);
	if (downl_onload != JNI_VERSION_1_6){
		return JNI_ERR;
	}

	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}

	jclass _class = (*env)->FindClass(env,"com/example/testandroidapp/MainActivity");
	if (!_class){
		return JNI_ERR;
	}

	(*env)->RegisterNatives(env, _class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );
}
