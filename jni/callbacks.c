#include "com_example_testandroidapp_MyDownloader.h"
#include <android/log.h>

static void writeCallback (JNIEnv *pEnv, jobject pThis, jint size){
	__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Downloaded");
	return;
}

static void progressCallback (JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jint sizeTotal, jint sizeCurr){
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
	{"writeCallback", "(I)V", (void *)writeCallback},
	{"progressCallback", "([BII)V", (void*)progressCallback}
};

jint JNI_OnLoad(JavaVM *vm, void *reserved){
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}
	jclass _class = (*env)->FindClass(env,"com/example/testandroidapp/MyDownloader");

	if (_class){
		(*env)->RegisterNatives(env, _class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );
	} else {
		return JNI_ERR;
	}
	return JNI_VERSION_1_6;
}
