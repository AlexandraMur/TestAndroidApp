#include "com_example_testandroidapp_MainActivity.h"
#include <pthread.h>
#include <android/log.h>

static JavaVM *globalVm;
static jclass globalMyDownloaderID;
static jmethodID globalDownloadID;
static jobject globalMyDownloaderObj;

static void workFlow (){
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

static void nativeTest (){
	pthread_t thread;
	pthread_create(&thread, NULL, (void*)workFlow, NULL);
}

static JNINativeMethod methodTable[] = {
	{"nativeTest", "()V", (void *)nativeTest}
};

jint JNI_OnLoad (JavaVM *vm_, void *reserved){
	if (!vm_){
		return JNI_ERR;
	}
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

	globalMyDownloaderObj = (*env)->NewGlobalRef(env, myDownloaderObj);
	if (!globalMyDownloaderObj){
		return JNI_ERR;
	}

	globalDownloadID = (*env)->GetMethodID(env, globalMyDownloaderID, "download", "(Ljava/lang/String;J)V");
	if (!globalDownloadID){
		return JNI_ERR;
	}

	jclass _class = (*env)->FindClass(env,"com/example/testandroidapp/MainActivity");
	if (!_class){
		return JNI_ERR;
	}

	(*env)->RegisterNatives(env, _class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );

	__android_log_write(ANDROID_LOG_INFO, "test.c", "GOOD");
	return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM *vm, void *reserved){

	if (!globalMyDownloaderObj){
		return;
	}

	JNIEnv* env;
	if ((*globalVm)->GetEnv(globalVm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return;
	}
	(*env)->DeleteGlobalRef(env, globalMyDownloaderObj);
}
