#ifndef METHODENTRY_H_
#define METHODENTRY_H_

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread,
            jmethodID method);

#endif /* METHODENTRY_H_ */