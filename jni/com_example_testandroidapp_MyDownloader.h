/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_example_testandroidapp_MyDownloader */

#ifndef _Included_com_example_testandroidapp_MyDownloader
#define _Included_com_example_testandroidapp_MyDownloader
#ifdef __cplusplus
extern "C" {
#endif
#undef com_example_testandroidapp_MyDownloader_BUFFER_SIZE
#define com_example_testandroidapp_MyDownloader_BUFFER_SIZE 4096L
/*
 * Class:     com_example_testandroidapp_MyDownloader
 * Method:    writeCallback
 * Signature: ([BLjava/lang/Integer;)V
 */
JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_writeCallback
  (JNIEnv *, jobject, jbyteArray, jobject);

/*
 * Class:     com_example_testandroidapp_MyDownloader
 * Method:    progressCallback
 * Signature: (Ljava/lang/Integer;Ljava/lang/Integer;)V
 */
JNIEXPORT void JNICALL Java_com_example_testandroidapp_MyDownloader_progressCallback
  (JNIEnv *, jobject, jobject, jobject);

#ifdef __cplusplus
}
#endif
#endif
