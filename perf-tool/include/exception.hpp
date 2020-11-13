#ifndef EXCEPTION_H_
#define EXCEPTION_H_


JNIEXPORT void JNICALL Exception(jvmtiEnv *jvmtiEnv,
            JNIEnv* jniEnv,
            jthread thread,
            jmethodID method,
            jlocation location,
            jobject exception,
            jmethodID catch_method,
            jlocation catch_location);

void setExceptionBackTrace(bool val);

void setExceptionSampleRate(int rate);

#endif /* EXCEPTION_H_ */
