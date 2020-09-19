#include <jvmti.h>

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread) {
    printf("VM starting up\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    printf("VM shutting down\n");
}