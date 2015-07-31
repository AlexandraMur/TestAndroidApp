#include "com_example_testandroidapp_MainActivity.h"
#include <pthread.h>
#include <android/log.h>

JavaVM *vm;

void JNICALL workFlow(JNIEnv *pEnv){

	__android_log_write(ANDROID_LOG_INFO, "test.c", "All works!");
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
