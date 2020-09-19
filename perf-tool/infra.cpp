#include <jvmti.h>
#include <server.hpp>

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread) {
    startServer(9003);
    sendMessageToClients("VM init");
    printf("VM starting up\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    sendMessageToClients("VM shutting down");
    shutDownServer();
    printf("VM shutting down\n");
}