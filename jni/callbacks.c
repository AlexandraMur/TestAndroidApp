#include "com_example_testandroidapp_MyDownloader.h"
#include <android/log.h>

JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_writeCallback
	(JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jint size){
	__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Downloaded");
	return;
}

JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_progressCallback
  (JNIEnv *pEnv, jobject pThis, jint sizeTotal, jint sizeCurr){
	if (sizeTotal == 0){
		//print only current size;
		__android_log_write(ANDROID_LOG_INFO, "Callbacks", "No current size");
	} else {
		//print percents
		int currPercent = (sizeCurr * 100) / sizeTotal;
		__android_log_write(ANDROID_LOG_INFO, "Callbacks", "Progress Callback");
	}
	return;
}
