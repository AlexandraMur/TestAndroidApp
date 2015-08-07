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

static void writeCallback (JNIEnv *pEnv, jobject pThis, jint size, jlong args){
	__android_log_write(ANDROID_LOG_INFO, "test.c", "Downloaded");
	return;
}

static void progressCallback (JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jint sizeTotal, jint sizeCurr, jlong args){
	if (sizeTotal == 0){
		//print only current size;
		__android_log_write(ANDROID_LOG_INFO, "test.c", "Current size");
	} else {
		//print percents
		int currPercent = (sizeCurr * 100) / sizeTotal;
		__android_log_write(ANDROID_LOG_INFO, "test.c", "Progress Callback");
	}
	return;
}

static void workFlow (){
	IDownloader_Cb my_callbacks;
	my_callbacks.complete = &writeCallback;
	my_callbacks.progress = &progressCallback;

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

	const char *url = "http://bakhirev.biz/book/index.html";
	const char *name = "test2.txt";

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

	__android_log_write(ANDROID_LOG_INFO, "test.c", "JNI_OnLoad");
	return JNI_VERSION_1_6;
}
