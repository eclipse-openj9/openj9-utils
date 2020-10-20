#include <jvmti.h>
#include <thread>
#include "server.hpp"
#include <infra.h>

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str;
        errnum_str = NULL;
        (void)jvmti->GetErrorName(errnum, &errnum_str);
        printf("ERROR: JVMTI: [%d] %s - %s", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        throw "Oops!";
    }
}


JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread) {
    startServer(8081);
    sendMessageToClients("VM init");
    printf("VM starting up\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    sendMessageToClients("VM shutting down");
    shutDownServer();

    printf("VM shutting down\n");
}