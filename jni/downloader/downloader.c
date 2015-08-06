#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#if USE_CURL
#include "curl/curl.h"
#endif //USE_CURL
#include <pthread.h>
#include <sys/queue.h>
#include <stdint.h>
#include <stdio.h>
#include "downloader.h"
#if defined(ANDROID) && !defined(USE_CURL)
#include <android/log.h>
#endif //defined(ANDROID) && !defined(USE_CURL)

struct Downloader {
	FILE *file;
	int alive;
    pthread_mutex_t mutex;
    pthread_cond_t conditional_variable;
    pthread_t thread;
    struct tailhead *headp;
    TAILQ_HEAD(tailhead, entry) head;
	IDownloader_Cb *my_callbacks;
	void *args;
	bool thread_flag;
	bool mutex_flag;
	bool cv_flag;
	struct entry *_entry;
	size_t queue_size;
};

struct entry {
    char *url;
    char *name_of_file;
    TAILQ_ENTRY(entry) entries;
};

#if ANDROID
static JavaVM *globalVm;
static jclass globalMyDownloaderID;
static jmethodID globalDownloadID;
static jobject globalMyDownloaderObj;
#endif //ANDROID

#if ANDROID
static void writeCallback (JNIEnv *pEnv, jobject pThis, jint size, jlong args){
	__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Downloaded");
	return;
}
#endif //ANDROID

#if ANDROID
static void progressCallback (JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jint sizeTotal, jint sizeCurr, jlong args){
	if (sizeTotal == 0){
		//print only current size;
		__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Current size");
	} else {
		//print percents
		int currPercent = (sizeCurr * 100) / sizeTotal;
		__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Progress Callback");
	}
	return;
}
#endif //ANDROID

#if ANDROID
static JNINativeMethod methodTable[] = {
	{"writeCallback", "(IJ)V", (void *)writeCallback},
	{"progressCallback", "([BIIJ)V", (void*)progressCallback}
};
#endif //ANDROID

#if defined(ANDROID) && !defined(USE_CURL)
int downloader_OnLoad(JavaVM *vm_){
	globalVm = vm_;

	JNIEnv* env;
	if ((*globalVm)->GetEnv(globalVm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}

	globalMyDownloaderID = (*env)->FindClass(env, "com/example/testandroidapp/MyDownloader");
	if (!globalMyDownloaderID){
		return JNI_ERR;
	}

	jclass myDownloaderObj = (*env)->AllocObject(env, globalMyDownloaderID);
	if (!myDownloaderObj){
		return JNI_ERR;
	}

	jclass _class = (*env)->FindClass(env,"com/example/testandroidapp/MyDownloader");

	if (_class){
		(*env)->RegisterNatives(env, _class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );
	} else {
		return JNI_ERR;
	}

	globalMyDownloaderObj = (*env)->NewGlobalRef(env, myDownloaderObj);
	if (!globalMyDownloaderObj){
		return JNI_ERR;
	}

	globalDownloadID = (*env)->GetMethodID(env, globalMyDownloaderID, "download", "(Ljava/lang/String;J)V");
	if (!globalDownloadID){
		return JNI_ERR;
	}

	return JNI_VERSION_1_6;
}
#endif //defined(ANDROID) && !defined(USE_CURL)

static void clear_queue(Downloader *d){
	pthread_mutex_lock(&d->mutex);
	while (TAILQ_FIRST(&d->head)){
		struct entry *_entry = TAILQ_FIRST(&d->head);
		TAILQ_REMOVE(&d->head, TAILQ_FIRST(&d->head), entries);
		free(_entry);
	}
	pthread_mutex_unlock(&d->mutex);
}

#if USE_CURL
static size_t callback(void *ptr, size_t size, size_t nmemb, Downloader* d){
    size_t _size = 0;
    if (d->alive){
            _size = fwrite(ptr, size, nmemb, d->file);
    }
    return _size;
}
#endif //USE_CURL

static void destroy_entry(struct entry *_entry){
	if (!_entry){
		return;
	}

	free(_entry->url);
	_entry->url = NULL;
	free(_entry->name_of_file);
	_entry->name_of_file = NULL;
	free(_entry);
	_entry = NULL;
}

struct entry* create_entry(char *url, char *name_of_file){
	struct entry *_entry = malloc(sizeof(struct entry));
	if (!_entry){
		return NULL;
	}
	_entry->url = strdup(url);
	_entry->name_of_file = strdup(name_of_file);

	if (!_entry->url || !_entry->name_of_file){
		destroy_entry(_entry);
	}
	return _entry;
}

#if USE_CURL
static int progress_callback(void *_d, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow){
	Downloader* d = (Downloader*)_d;
		
	if (dlnow == 0 || dltotal ==0){
		return 0;
	}
	if (d->my_callbacks->progress){
		d->my_callbacks->progress(d, d->args, d->_entry->url, d->_entry->name_of_file, dlnow, dltotal);
	}
  	return 0;
}
#endif //USE_CURL

#if USE_CURL
static void download(Downloader *d) {

	int error = DOWNLOADER_STATUS_ERROR;
	CURL *curl = curl_easy_init();
	if (!curl) {
		goto exit;
	}

	d->file = NULL;
        d->file = fopen(d->_entry->name_of_file,"wb");
        
	if(!d->file) {
		goto exit;
	}
	
	curl_easy_setopt(curl, CURLOPT_URL, d->_entry->url);	
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, d);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, d);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	CURLcode code = curl_easy_perform(curl);
    fclose(d->file);

	if(code){
		remove(d->_entry->name_of_file);
		error = DOWNLOADER_STATUS_ERROR;
	}
	error = DOWNLOADER_STATUS_OK;
exit:
	if (curl){
		curl_easy_cleanup(curl);
		curl = NULL;		
	}
	if (d->my_callbacks->complete) {
    	d->my_callbacks->complete(d, d->args, d->_entry->url, d->_entry->name_of_file, error, d->queue_size);
	}
}
#else
static void download(Downloader *d) {
	JNIEnv *pEnv = NULL;
	if ((*globalVm)->AttachCurrentThread(globalVm, &pEnv, NULL) != JNI_OK){
		goto exit;
	}
	jstring jStr = (*pEnv)->NewStringUTF(pEnv, d->_entry->url);
	long args = NULL;
	(*pEnv)->CallVoidMethod(pEnv, globalMyDownloaderObj, globalDownloadID, jStr, args);
exit:
	(*globalVm)->DetachCurrentThread(globalVm);
}
#endif //USE_CURL

static void *work_flow(void* _d){
	Downloader* d = (Downloader*)_d;
    while(1) {
		pthread_mutex_lock(&d->mutex);
        while (TAILQ_EMPTY(&d->head) && d->alive){
				pthread_cond_wait(&d->conditional_variable, &d->mutex);
        }

        if (!d->alive) {
        	pthread_mutex_unlock(&d->mutex);
            break;
        }

   		d->_entry = TAILQ_FIRST(&d->head);
		TAILQ_REMOVE(&d->head, d->_entry, entries);
		d->queue_size--;
		pthread_mutex_unlock(&d->mutex);
		download(d);
		destroy_entry(d->_entry);
    }
    return NULL;
}

Downloader *downloader_create(IDownloader_Cb *my_callbacks, void *args){
	if (!my_callbacks) {
		return NULL;
	}
	Downloader *d = calloc(1, sizeof(struct Downloader));
	if (!d){
		goto fail;
	}
	d->alive = 1;
	d->my_callbacks = my_callbacks;
	d->args = args;
	d->queue_size = 0;	 
	TAILQ_INIT(&d->head);
		
	d->mutex_flag = !pthread_mutex_init(&d->mutex, NULL);
    d->cv_flag = !pthread_cond_init(&d->conditional_variable, NULL);
	d->thread_flag = !pthread_create(&d->thread, NULL, work_flow, (void*)d);
	if (!d->mutex_flag || !d->cv_flag || !d->thread_flag){
		goto fail;
	}
	return d;

fail:
	downloader_destroy(d);
	return NULL;
} 

void downloader_destroy(Downloader *d) {
	if (!d) {
		return;
	}

	if (d->thread_flag) {
		pthread_mutex_lock(&d->mutex);
		d->alive = 0;
		pthread_cond_broadcast(&d->conditional_variable);
		pthread_mutex_unlock(&d->mutex);
		pthread_join(d->thread, NULL);
		d->thread_flag = 0;
	}
	
	if(d->mutex_flag){
		pthread_mutex_destroy(&d->mutex);
		d->mutex_flag = 0;
	}
	if(d->cv_flag){
		pthread_cond_destroy(&d->conditional_variable);
		d->cv_flag = 0;
	}

	clear_queue(d);
	free(d);	
}

int downloader_add(Downloader *d, char *url, char *name_of_file) {
	if (!d) {
		return DOWNLOADER_STATUS_ERROR;
	}
	struct entry *_entry = create_entry(url, name_of_file);
	if(!_entry) {
		return DOWNLOADER_STATUS_ERROR;
	}
	pthread_mutex_lock(&d->mutex);
	TAILQ_INSERT_TAIL(&d->head, _entry, entries);
	d->queue_size++;
	pthread_cond_broadcast(&d->conditional_variable);
	pthread_mutex_unlock(&d->mutex);

	return DOWNLOADER_STATUS_OK;
}
