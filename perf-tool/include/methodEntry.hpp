#ifndef METHODENTRY_H_
#define METHODENTRY_H_

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread,
            jmethodID method);

void setMethodEntrySampleRate(int rate);

#endif /* METHODENTRY_H_ */