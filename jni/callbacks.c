#include "com_example_testandroidapp_MyDownloader.h"

JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_writeCallback
	(JNIEnv *pEnv, jobject pThis, jbyteArray byteArray, jobject size){
	return;
}

JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_progressCallback
  (JNIEnv *pEnv, jobject pThis, jobject sizeTotal, jobject sizeCurr){
	return;
}
