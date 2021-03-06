#if defined(ANDROID) && !defined(USE_CURL)

#include "http_client.h"

#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <assert.h>

#include <stdbool.h>

#define LOG_TAG	"http_client"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)

typedef enum {
	DOWNLOADER_STATUS_ERROR = -1,
	DOWNLOADER_STATUS_OK = 0,
	DOWNLOADER_STATUS_URL_ERROR,
	DOWNLOADER_STATUS_CONNECTION_TIMEOUT_ERROR,
	DOWNLOADER_STATUS_RECIEVEDATA_TIMEOUT_ERROR
} MyDownloaderStatus;

struct HttpClient
{
	const IHttpClientCb *cb;
	void *arg;
	int timeout_connection;
	int timeout_recieve;
	bool shutdown;
};

static jmethodID g_method_download;
static jclass g_class_MyDownloader;
static JavaVM *g_vm;

void http_client_set_timeout_connection (HttpClient *c, int timeout)
{
	assert(c);
	c->timeout_connection = timeout;
}

void http_client_set_timeout_recieve (HttpClient *c, int timeout)
{
	assert(c);
	c->timeout_recieve = timeout;
}

int http_client_get_timeout_connection (HttpClient *c)
{
	assert(c);
	return c->timeout_connection;
}

int http_client_get_timeout_recieve (HttpClient *c)
{
	assert(c);
	return c->timeout_recieve;
}

bool isShutdown (JNIEnv *env, jobject obj, jlong args)
{
	HttpClient *client = (HttpClient*)((intptr_t)args);
	assert(client);
	return client->shutdown;
}

static HttpClientStatus MyDownloaderStatus_to_HttpClientStatus(MyDownloaderStatus status)
{
	switch(status){
	case DOWNLOADER_STATUS_OK:
		return HTTP_CLIENT_OK;
	case DOWNLOADER_STATUS_URL_ERROR:
		return HTTP_CLIENT_BAD_URL;
	case DOWNLOADER_STATUS_CONNECTION_TIMEOUT_ERROR:
		return HTTP_CLIENT_TIMEOUT_CONNECT;
	case DOWNLOADER_STATUS_RECIEVEDATA_TIMEOUT_ERROR:
		return HTTP_CLIENT_TIMEOUT_RECIEVE;
	default:
		return HTTP_CLIENT_ERROR;
	}
}

static void writeCallback (JNIEnv *env, jobject obj, jbyteArray byte_array, jint size, jlong args)
{
	HttpClient *client = (HttpClient*)((intptr_t)args);
	assert(client);

	jbyte* buffer_ptr = (*env)->GetByteArrayElements(env, byte_array, NULL);

	if (client->cb->data) {
		client->cb->data(client, (void*)client->arg, buffer_ptr, size);
	}

	(*env)->ReleaseByteArrayElements(env, byte_array, buffer_ptr, 0);
}

static void progressCallback (JNIEnv *env, jobject obj, jint total_size, jint curr_size, jlong args)
{
	HttpClient *client = (HttpClient*)((intptr_t)args);
	assert(client);
	if (client->cb->progress) {
		client->cb->progress(client, client->arg, total_size, curr_size);
	}
}

HttpClient* http_client_create (const IHttpClientCb *cb, void* arg)
{
	if (!cb) {
		return NULL;
	}
	HttpClient *c = calloc(1, sizeof(*c));
	if (!c) {
		return NULL;
	}
	c->cb = cb;
	c->arg = arg;
	c->shutdown = false;
	return c;
}

HttpClientStatus http_client_download (HttpClient *c, const char *url)
{
	if (!c || !url) {
		return HTTP_CLIENT_ERROR;
	}
	c->shutdown = false;
	HttpClientStatus result = HTTP_CLIENT_INSUFFICIENT_RESOURCE;
	JNIEnv *pEnv;
	if ((*g_vm)->AttachCurrentThread(g_vm, &pEnv, NULL) != JNI_OK) {
		LOGE("AttachCurrentThread failed\n");
		return result;
	}

	if ((*g_vm)->GetEnv(g_vm, (void**)(&pEnv), JNI_VERSION_1_6) != JNI_OK) {
			return JNI_ERR;
	}

	jclass obj_MyDownloader = (*pEnv)->AllocObject(pEnv, g_class_MyDownloader);
	if (!obj_MyDownloader) {
		LOGE("AllocObject failed\n");
		return result;
	}

	jstring jurl = (*pEnv)->NewStringUTF(pEnv, url);
	if (!jurl) {
		LOGE("NewStringUTF failed\n");
		goto done;
	}

	LOGI("Start download %s\n", url);
	MyDownloaderStatus downloader_result = (*pEnv)->CallIntMethod(pEnv, obj_MyDownloader, g_method_download, jurl, c->timeout_connection, c->timeout_recieve, (jlong)c);
	(*pEnv)->DeleteLocalRef(pEnv, jurl);
	result = MyDownloaderStatus_to_HttpClientStatus(downloader_result);
done:
	if (obj_MyDownloader) {
		(*pEnv)->DeleteLocalRef(pEnv, obj_MyDownloader);
	}
	return result;
}

void http_client_reset (HttpClient *c)
{
	assert(c);
	c->shutdown = true;
}

void http_client_destroy (HttpClient *c)
{
	assert(c);
	free(c);
}

static JNINativeMethod methodTable[] =
{
	{ "writeCallback",    "([BIJ)V", (void*)writeCallback },
	{ "progressCallback", "(IIJ)V",  (void*)progressCallback },
	{ "isShutdown",		  "(J)Z", 	 (void*)isShutdown }
};

int http_client_on_load (JavaVM *vm_)
{
	g_vm = vm_;
	JNIEnv* env;
	if ((*g_vm)->GetEnv(g_vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}

	jclass tmp = (*env)->FindClass(env, "com/example/testandroidapp/MyDownloader");
	if (!tmp){
		return JNI_ERR;
	}

	g_class_MyDownloader = (*env)->NewGlobalRef(env, tmp);
	if (!g_class_MyDownloader){
		return JNI_ERR;
	}

	g_method_download = (*env)->GetMethodID(env, g_class_MyDownloader, "download", "(Ljava/lang/String;IIJ)I");
	if (!g_method_download){
		return JNI_ERR;
	}

	if (JNI_OK != (*env)->RegisterNatives(env, g_class_MyDownloader, methodTable, sizeof(methodTable) / sizeof(methodTable[0]))) {
		return JNI_ERR;
	}
	return JNI_OK;
}

void http_client_android_detach(void)
{
	(*g_vm)->DetachCurrentThread(g_vm);
}

#endif //defined(ANDROID) && !defined(USE_CURL)
