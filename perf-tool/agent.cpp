#include <jvmti.h>
#include <string.h>

#include "infra.h"
#include "monitor.h"
#include "json.hpp"

using json = nlohmann::json;

static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str;
        errnum_str = NULL;
        (void)jvmti->GetErrorName(errnum, &errnum_str);
        printf("ERROR: JVMTI: [%d] %s - %s", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        throw "Oops!";
    }
}

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method) {
    /*jvmtiError err;
    json j;
    char *methodName;
    err = jvmtiEnv->GetMethodName(method, &methodName, NULL, NULL);
    if (err == JVMTI_ERROR_NONE && strcmp(methodName, "doBatch") == 0) {
        j["methodName"] = methodName;
        std::string s = j.dump();

        printf("\n%s\n", s.c_str());
    }*/
}


JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv, 
                        JNIEnv* env, 
                        jthread thread, 
                        jobject object, 
                        jclass object_klass, 
                        jlong size) {
    jvmtiError err;
    // jobject cls = env->GetObjectClass(object);
    // jobjectRefType clsType = env->GetObjectRefType(object_klass);
    json j;

    // err = jvmtiEnv->GetClassSignature(object_klass, &refType, NULL);
    // if (err == JVMTI_ERROR_NONE) {
    //     j["classType"] = refType;
    //     std::string s = j.dump();
    //     printf("\n%s\n", s.c_str());
    // }

    jint class_count;
    jclass* classes;

    err = jvmtiEnv->GetLoadedClasses(&class_count, &classes);
    if (err == JVMTI_ERROR_NONE && class_count >= 1) {
        for (int i = 0; i < class_count; i++) {
            char * refType;
            err = jvmtiEnv->GetClassSignature(classes[i], &refType, NULL);
            if (err == JVMTI_ERROR_NONE) {
                j["refNum"] = i;
                j["refType"] = refType;
            }
        }
    }

    // jint * hcode;
    // err = jvmtiEnv->GetObjectHashCode(object, &hcode);
    // if (err == JVMTI_ERROR_NONE) {
    //     j["address"] = hcode;
    // }
    
    std::string s = j.dump();
    printf("\n%s\n", s.c_str());


}



JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    jvmtiEnv *jvmti;
    jint rest = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (rest != JNI_OK || jvmti == NULL) {

        printf("Unable to get access to JVMTI version 1.0");
        return JNI_ERR;
    }

    jvmtiCapabilities capa;
    jvmtiError error;
    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    capa.can_generate_monitor_events = 1;
    capa.can_generate_vm_object_alloc_events = 1;
    capa.can_generate_method_entry_events = 1;
    error = jvmti->AddCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Failed to set jvmtiCapabilities.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM init event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM death event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init MonitorContendedEntered event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE,  JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init Method Entry event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM Object Alloc event");

    jvmtiEventCallbacks callbacks;
    (void)memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
    callbacks.VMInit = &VMInit;
    callbacks.VMDeath = &VMDeath;
    callbacks.MonitorContendedEntered = &MonitorContendedEntered;
    callbacks.MethodEntry = &MethodEntry;
    callbacks.VMObjectAlloc = &VMObjectAlloc;
    error = jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    return JNI_OK;
}
