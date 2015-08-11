#if defined(ANDROID) && !defined(USE_CURL)
#include <stdio.h>
#include "http_client.h"
#include <jni.h>
#include <android/log.h>

struct HttpClient {
	FILE *file;
	const char *name;
	const char *url;
	IHttpClientCb *cb;
};

static const char TAG[] = "HTTP CLIENT ANDROID";
static JavaVM *globalVm;
static jclass globalMyDownloaderID;
static jmethodID globalDownloadID;

static void writeCallback (JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jint size, jlong args){
	HttpClient *http_client = (HttpClient*)args;
	if (!http_client){
		return;
	}
	http_client->cb->data(http_client, (void*)http_client->name, byteArray, size, http_client->file);
	return;
}

static void progressCallback (JNIEnv *pEnv, jobject pThis, jint sizeTotal, jint sizeCurr, jlong args){
	if (sizeTotal){
		__android_log_write(ANDROID_LOG_INFO, TAG, "percents");
	} else{
		__android_log_write(ANDROID_LOG_INFO, TAG, "Current size");
	}
}

static JNINativeMethod methodTable[] = {
	{"writeCallback", "([BIJ)V", (void *)writeCallback},
	{"progressCallback", "(IIJ)V", (void*)progressCallback}
};

HttpClient* http_client_create (IHttpClientCb *cb, void* args){
	if (!cb){
		return NULL;
	}
	HttpClient *c = calloc(1, sizeof(struct HttpClient));
	c->cb = cb;
	return c;
}


HttpClientStatus http_client_download (HttpClient *c, const char *url, const char *name_of_file){
	HttpClientStatus result = HTTP_CLIENT_FAIL;
	if (!c){
		return result;
	}
	c->name = name_of_file;
	c->url = url;
	jclass obj = NULL;
	JNIEnv *pEnv = NULL;
	if ((*globalVm)->AttachCurrentThread(globalVm, &pEnv, NULL) != JNI_OK){
		goto exit;
	}

	obj = (*pEnv)->AllocObject(pEnv, globalMyDownloaderID);
	if (!obj){
		return JNI_ERR;
	}

	jstring jStr = (*pEnv)->NewStringUTF(pEnv, url);
	c->file = fopen(name_of_file, "wb");
	if (!c->file){
		goto exit;
	}

	long args = (long)c;
	if ((*pEnv)->CallIntMethod(pEnv, obj, globalDownloadID, jStr, args) == HTTP_CLIENT_OK){
		__android_log_write(ANDROID_LOG_INFO, TAG, "OK");
		result = HTTP_CLIENT_OK;
	}
exit:
	if (c->file){
		fclose(c->file);
	}
	if (obj){
		(*pEnv)->DeleteLocalRef(pEnv, obj);
		__android_log_write(ANDROID_LOG_INFO, TAG, "2 OK");
	}
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

	jclass tmp = (*env)->FindClass(env, "com/example/testandroidapp/MyDownloader");
	if (!tmp){
		return JNI_ERR;
	}

	globalMyDownloaderID = (*env)->NewGlobalRef(env, tmp);
	if (!globalMyDownloaderID){
		return JNI_ERR;
	}

	globalDownloadID = (*env)->GetMethodID(env, globalMyDownloaderID, "download", "(Ljava/lang/String;J)I");
	if (!globalDownloadID){
		return JNI_ERR;
	}

	(*env)->RegisterNatives(env, globalMyDownloaderID, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );

	return JNI_VERSION_1_6;
}

void http_client_android_detach(void){
	(*globalVm)->DetachCurrentThread(globalVm);
}

#endif //defined(ANDROID) && !defined(USE_CURL)
