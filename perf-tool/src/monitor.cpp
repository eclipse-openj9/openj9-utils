#include <atomic>
#include <jvmti.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"

//how many times 
using json = nlohmann::json;

std::atomic<bool> backTraceEnabled;

void setStackTrace(bool val){
    // Enables or disables the stack trace option
    backTraceEnabled = val;
    return;
}

JNIEXPORT void JNICALL MonitorContendedEntered(jvmtiEnv *jvmtiEnv, JNIEnv* env, jthread thread, jobject object) {
    json j;
    static int numContentions = 0; //number of total contentions that have been recorded
    numContentions++;
    jvmtiError error;

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
    /*char *str;    
    error = jvmtiEnv->GetClassSignature(cls , &str, NULL);
    if (str != NULL && error == JVMTI_ERROR_NONE) */

    // record calling class
    j["Occurence"]["Class"] = str;

    // Release the memory pinned char array
    env->ReleaseStringUTFChars(strObj, str);

    if(backTraceEnabled){ // only run if the backtrace is enabled
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
    }

    j["Contention Number"] = numContentions;

    // printf("%s\n", j.dump().c_str());
    
    sendToServer(j.dump());
}



