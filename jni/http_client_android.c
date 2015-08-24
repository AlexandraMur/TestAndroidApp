#if defined(ANDROID) && !defined(USE_CURL)

#include "http_client.h"

#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <assert.h>

#define LOG_TAG	"http_client"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)

struct HttpClient
{
	const IHttpClientCb *cb;
	void *arg;
	int timeout;
	int shutdown;
};

static JavaVM *g_vm;
static jclass g_class_MyDownloader;
static jmethodID g_method_download;

void http_client_set_timeout(HttpClient *c, int timeout)
{	if (!c){
		return;
	}
	c->timeout = timeout;
}

int get_timeout(HttpClient *c)
{
	return c->timeout;
}
static int writeCallback (JNIEnv *env, jobject obj, jbyteArray byte_array, jint size, jlong args)
{
	HttpClient *client = (HttpClient*)((intptr_t)args);
	assert(client);

	if (client->shutdown){
		LOGI("SHUTDOWN");
		return 1;
	}

	jbyte* buffer_ptr = (*env)->GetByteArrayElements(env, byte_array, NULL);

	if (client->cb->data) {
		client->cb->data(client, (void*)client->arg, buffer_ptr, size);
	}

	(*env)->ReleaseByteArrayElements(env, byte_array, buffer_ptr, 0);
	return 0;
}

static void progressCallback (JNIEnv *env, jobject obj, jint total_size, jint curr_size, jlong args)
{
	HttpClient *client = (HttpClient*)((intptr_t)args);
	assert(client);
	if (client->shutdown){
			return;
	}
	if (client->cb->progress) {
		client->cb->progress(client, client->arg, total_size, curr_size);
	}
}

static JNINativeMethod methodTable[] =
{
	{ "writeCallback",    "([BIJ)I", (void*)writeCallback },
	{ "progressCallback", "(IIJ)V",  (void*)progressCallback }
};

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
	c->shutdown = 0;
	return c;
}


HttpClientStatus http_client_download (HttpClient *c, const char *url)
{
	if (!c || !url) {
		LOGE("Invalid args\n");
		return HTTP_CLIENT_INVALID_ARG;
	}

	HttpClientStatus result = HTTP_CLIENT_INSUFFICIENT_RESOURCE;
	JNIEnv *pEnv;
	if ((*g_vm)->AttachCurrentThread(g_vm, &pEnv, NULL) != JNI_OK) {
		LOGE("AttachCurrentThread failed\n");
		return result;
	}

	if ((*g_vm)->GetEnv(g_vm, (void**)(&pEnv), JNI_VERSION_1_6) != JNI_OK) {
			return JNI_ERR;
	}

	jclass obj = (*pEnv)->AllocObject(pEnv, g_class_MyDownloader);
	if (!obj) {
		LOGE("AllocObject failed\n");
		return result;
	}
	jstring jurl = (*pEnv)->NewStringUTF(pEnv, url);
	if (!jurl) {
		LOGE("NewStringUTF failed\n");
		goto done;
	}
	LOGI("Start download %s\n", url);
	result = (*pEnv)->CallIntMethod(pEnv, obj, g_method_download, jurl, c->timeout, (jlong)c);
	(*pEnv)->DeleteLocalRef(pEnv, jurl);

done:
	if (obj) {
		(*pEnv)->DeleteLocalRef(pEnv, obj);
	}
	return result;
}

void http_client_reset (HttpClient *c)
{
	LOGI("RESET");
	if(!c){
		LOGI("C");
	}
	if (!g_vm){
		LOGI("VM");
	}

	//JNIEnv *pEnv;
	//if ((*g_vm)->AttachCurrentThread(g_vm, &pEnv, NULL) != JNI_OK) {
		//LOGE("AttachCurrentThread failed\n");
		//return;
	//}

	c->shutdown = 1;
}

void http_client_destroy (HttpClient *c)
{
	if (!c) {

		return;
	}
	free(c);
}

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
	g_method_download = (*env)->GetMethodID(env, g_class_MyDownloader, "download", "(Ljava/lang/String;IJ)I");
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
