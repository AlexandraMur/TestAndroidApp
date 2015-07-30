#include <pthread.h>
#include <android/log.h>
#include "com_example_testandroidapp_MainActivity.h"

void JNICALL workFlow(JNIEnv *pEnv){
	jclass myDownloader = (*pEnv)->FindClass(pEnv, "com/example/testandroidapp/MyDownloader");
	if (!myDownloader){
		__android_log_write(ANDROID_LOG_INFO, "test.c", "FAIL myDownloader");
		return;
	}

	jclass myClass = (*pEnv)->GetObjectClass(pEnv, myDownloader);

	if (!myClass){
		__android_log_write(ANDROID_LOG_INFO, "test.c", "FAIL myClass");
	}

	jmethodID downloadID = (*pEnv)->GetMethodID(pEnv, myClass, "download", "(Ljava/lang/String;)L");
	if (!downloadID){
		__android_log_write(ANDROID_LOG_INFO, "test.c", "FAIL downloadID");
	}

	//(*pEnv)->CallVoidMethod(pEnv, myClass, constructorID);
	//__android_log_write(ANDROID_LOG_INFO, "test.c", "constructor");

	// don't forget about arguments in download
	//(*pEnv)->CallVoidMethod(pEnv, myClass, downloadID);
	//__android_log_write(ANDROID_LOG_INFO, "test.c", "download");

}


JNIEXPORT void JNICALL Java_com_example_testandroidapp_MainActivity_nativeTest (JNIEnv *pEnv){
	workFlow(pEnv);
}
