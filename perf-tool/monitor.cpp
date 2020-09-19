#include <jvmti.h>
#include "json.hpp"
#include <server.hpp>

//how many times 
using json = nlohmann::json;

JNIEXPORT void JNICALL MonitorContendedEntered(jvmtiEnv *jvmtiEnv, JNIEnv* env, jthread thread, jobject object) {
    json j;
    static int numContentions = 0; // keep track of number of contentions

    numContentions++;

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

    // record calling class
    j["Occurence"]["Class"] = str;

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
            j["Occurence"]["Method"] = methodName;
        }
    }

    j["Num Contentions"] = numContentions;

    printf("%s\n", j.dump().c_str());
    sendMessageToClients(j.dump());

    /*jint method_count;
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
        
    }*/
}



