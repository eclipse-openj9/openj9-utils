#include <jvmti.h>
#include <thread>
#include <string.h>

#include "infra.h"
#include "server.hpp"

Server *server;

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str;
        errnum_str = NULL;
        (void)jvmti->GetErrorName(errnum, &errnum_str);
        printf("ERROR: JVMTI: [%d] %s - %s", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        throw "Oops!";
    }
}

jthread createNewThread(JNIEnv* jni_env){
    // allocates a new java thread object using JNI
    jclass threadClass;
    jmethodID id;
    jthread newThread;

    threadClass = jni_env->FindClass("java/lang/Thread");
    id = jni_env -> GetMethodID(threadClass, "<init>", "()V");
    newThread = jni_env -> NewObject(threadClass, id);
    return newThread;
}

void JNICALL startServer(jvmtiEnv * jvmti, JNIEnv* jni, void *p)
{
    server->handleServer();
}

void sendToServer(std::string message)
{
    server->messageQueue.push(message);
}

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread) {
    jvmtiError error;
    int* portPointer = portNo ? &portNo : NULL;

    server = new Server(portNo, commandsPath, logPath);

    error = jvmtiEnv -> RunAgentThread( createNewThread(jni_env),&startServer, portPointer, JVMTI_THREAD_NORM_PRIORITY );
    check_jvmti_error(jvmtiEnv, error, "Error starting agent thread\n");
    sendToServer("VM init");
    printf("VM starting up\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    server->shutDownServer();
    printf("VM shutting down\n");
}
