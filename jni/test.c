#include "com_example_testandroidapp_MainActivity.h"
#include <pthread.h>
#include <android/log.h>

JavaVM *vm;

void JNICALL workFlow(JNIEnv *pEnv){
	jclass myDownloader = (*pEnv)->FindClass(pEnv, "com/example/testandroidapp/MyDownloader");
	if (!myDownloader){
		__android_log_write(ANDROID_LOG_INFO, "test.c", "FAIL myDownloader");
		return;
	}

	jmethodID downloadID = (*pEnv)->GetStaticMethodID(pEnv, myDownloader, "download", "(Ljava/lang/String;)V");
	if (!downloadID){
		__android_log_write(ANDROID_LOG_INFO, "test.c", "FAIL downloadID");
		return;
	}

	jstring jStr = (*pEnv)->NewStringUTF(pEnv, "http://idev.by/android/22971/");
	(*pEnv)->CallStaticVoidMethod(pEnv, myDownloader, downloadID, jStr);
}

static void nativeTest (JNIEnv *pEnv){
		workFlow(pEnv);
}

static JNINativeMethod methodTable[] = {
	{"nativeTest", "()V", (void *)nativeTest}
};

jint JNI_OnLoad(JavaVM *vm_, void *reserved){
	vm = vm_;
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}
	jclass _class = (*env)->FindClass(env,"com/example/testandroidapp/MainActivity");

	if (_class){
		(*env)->RegisterNatives(env, _class, methodTable, sizeof(methodTable) / sizeof(methodTable[0]) );
	} else {
		return JNI_ERR;
	}
	return JNI_VERSION_1_6;
}
