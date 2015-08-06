#if defined(ANDROID) && !defined(USE_CURL)
#include "http_client.h"
#include <jni.h>
#include <android/log.h>

struct HttpClient {
	IHttpClientCb *cb;
};

static JavaVM *globalVm;
static jclass globalMyDownloaderID;
static jmethodID globalDownloadID;
static jobject globalMyDownloaderObj;

static void writeCallback (JNIEnv *pEnv, jobject pThis, jint size, jlong args){
	__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Downloaded");
	return;
}

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

static JNINativeMethod methodTable[] = {
	{"writeCallback", "(IJ)V", (void *)writeCallback},
	{"progressCallback", "([BIIJ)V", (void*)progressCallback}
};


HttpClient* http_client_create (IHttpClientCb *cb, void* args){
	if (!cb){
		return NULL;
	}
	HttpClient *c = calloc(1, sizeof(struct HttpClient));
	c->cb = cb;
	return c;
}


HttpClientStatus http_client_download (HttpClient *c, const char *url){
	HttpClientStatus result = HTTP_CLIENT_FAIL;
	JNIEnv *pEnv = NULL;
	if ((*globalVm)->AttachCurrentThread(globalVm, &pEnv, NULL) != JNI_OK){
		goto exit;
	}
	jstring jStr = (*pEnv)->NewStringUTF(pEnv, url);
	long args = NULL;
	(*pEnv)->CallVoidMethod(pEnv, globalMyDownloaderObj, globalDownloadID, jStr, args);
	result = HTTP_CLIENT_OK;
exit:
	(*globalVm)->DetachCurrentThread(globalVm);
	return result;
}

void http_client_reset (HttpClient *c){
	//TODO
}

void http_client_destroy (HttpClient *c){
	free(c);
}

int http_client_on_load (JavaVM *vm_){
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
