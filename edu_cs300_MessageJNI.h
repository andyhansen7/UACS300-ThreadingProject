/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class edu_cs300_MessageJNI */

#ifndef _Included_edu_cs300_MessageJNI
#define _Included_edu_cs300_MessageJNI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     edu_cs300_MessageJNI
 * Method:    writeReportRequest
 * Signature: (IILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_edu_cs300_MessageJNI_writeReportRequest
  (JNIEnv *, jclass, jint, jint, jstring);

/*
 * Class:     edu_cs300_MessageJNI
 * Method:    readReportRecord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_cs300_MessageJNI_readReportRecord
  (JNIEnv *, jclass, jint);

/*
 * Class:     edu_cs300_MessageJNI
 * Method:    readStringMsg
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_cs300_MessageJNI_readStringMsg
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif
