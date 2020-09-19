#include <jvmti.h>
#include <string.h>
#include <json.hpp>

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

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread) {
    printf("VM starting up\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    printf("VM shutting down\n");
}


JNIEXPORT void JNICALL MonitorContendedEntered(jvmtiEnv *jvmtiEnv, JNIEnv* env, jthread thread, jobject object) {
    printf("Contention entered\n");

    jclass cls = env->GetObjectClass(object);
    // First get the class object
    jmethodID mid = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = env->CallObjectMethod(object, mid);

    // Now get the class object's class descriptor
    cls = env->GetObjectClass(clsObj);

    // Find the getName() method on the class object
    mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");

    // Call the getName() to get a jstring object back
    jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);

    // Now get the c string from the java jstring object
    const char* str = env->GetStringUTFChars(strObj, NULL);

    // Print the class name
    printf("\nCalling class is: %s\n", str);

    // Release the memory pinned char array
    env->ReleaseStringUTFChars(strObj, str);

    jvmtiFrameInfo frames[5];
    jint count;
    jvmtiError err;

    err = jvmtiEnv->GetStackTrace(thread, 0, 5, 
                                frames, &count);
    if (err == JVMTI_ERROR_NONE && count >= 1) {
        char *methodName;
        err = jvmtiEnv->GetMethodName(frames[0].method, 
                            &methodName, NULL, NULL);
        if (err == JVMTI_ERROR_NONE) {
            printf("Executing method: %s", methodName);
        }
    }

    jint method_count;
    jmethodID *methodIDs;

    err = jvmtiEnv->GetClassMethods(cls, &method_count, &methodIDs);
    if (err == JVMTI_ERROR_NONE && method_count >= 1) {
        for (int i = 0; i < method_count; i++)
        {
            char *methodName;
            err = jvmtiEnv->GetMethodName(methodIDs[i], 
                                &methodName, NULL, NULL);
            if (err == JVMTI_ERROR_NONE) {
                printf("\nMethod #%i: %s\n", i, methodName);
            }
        }
        
    }
}


JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method) {
    jvmtiError err;
    json j;
    char *methodName;
    err = jvmtiEnv->GetMethodName(method, &methodName, NULL, NULL);
    if (err == JVMTI_ERROR_NONE && strcmp(methodName, "doBatch") == 0) {
        j["methodName"] = methodName;
        std::string s = j.dump();

        printf("\n%s\n", s.c_str());
    }
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

    jvmtiEventCallbacks callbacks;
    (void)memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
    callbacks.VMInit = &VMInit;
    callbacks.VMDeath = &VMDeath;
    callbacks.MonitorContendedEntered = &MonitorContendedEntered;
    callbacks.MethodEntry = &MethodEntry;
    error = jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    return JNI_OK;
}
