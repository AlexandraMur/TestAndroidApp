#include "com_example_testandroidapp_MyDownloader.h"
#include <android/log.h>

JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_writeCallback
	(JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jobject size){
	__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Downloaded");
	return;
}

JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_progressCallback
  (JNIEnv *pEnv, jobject pThis, jobject sizeTotal, jobject sizeCurr){
	__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Progress Callbsck");
	return;
}
