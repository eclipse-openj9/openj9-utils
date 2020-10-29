#include <atomic>
#include <jvmti.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"


using json = nlohmann::json;

std::atomic<bool> backTraceEnabled;

void setExceptionBackTrace(bool val){
    // Enables or disables the stack trace option
    backTraceEnabled = val;
    return;
}

JNIEXPORT void JNICALL Exception(jvmtiEnv *jvmtiEnv,
            JNIEnv* jniEnv,
            jthread thread,
            jmethodID method,
            jlocation location,
            jobject exception,
            jmethodID catch_method,
            jlocation catch_location) {

    json jdata;
    jvmtiError err;

    static int numExceptions = 0; //number of total contentions that have been recorded
    numExceptions++;

    // First get exception...how to convert pointer to c string?
    char ptrStr[] = "%p";
    char buffer[20];
    sprintf(buffer, ptrStr, exception);
    jdata["exception"] = buffer;


    char *methodName;
    err = jvmtiEnv->GetMethodName(method,
                        &methodName, NULL, NULL);

    // record calling method
    jdata["exception"]["callingMethod"] = methodName;

    if(backTraceEnabled){ // only run if the backtrace is enabled
        jvmtiFrameInfo frames[5];
        jint count;
        jvmtiError err;
        auto jMethods = json::array();

        err = jvmtiEnv->GetStackTrace(thread, 0, 5,
                                    frames, &count);
        if (err == JVMTI_ERROR_NONE && count >= 1) {
            char *methodName;
            json jMethod;
            err = jvmtiEnv->GetMethodName(frames[0].method,
                                &methodName, NULL, NULL);
            if (err == JVMTI_ERROR_NONE) {
                jMethod["methodName"] = methodName;
                jMethods.push_back(jMethod);
            }
        }

        jdata["exception"]["backtrace"] = jMethods;
    }

    jdata["numExceptions"] = numExceptions;

    sendToServer(jdata.dump());
}
