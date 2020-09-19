#ifndef MONITOR_H_
#define MONITOR_H_

JNIEXPORT void JNICALL MonitorContendedEntered(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread, 
            jobject object);

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method);



#endif /* MONITOR_H_ */ 