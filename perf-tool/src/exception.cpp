#include <atomic>
#include <jvmti.h>
#include <string>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

std::atomic<bool> backTraceEnabled;

void setExceptionBackTrace(bool val){
    // Enables or disables the stack trace option
    backTraceEnabled = val;
    return;
}

// const char* pointerToChar(jobject exception) {
//     // First get exception...how to convert pointer to c string?
//     char ptrStr[] = "%p";
//     char* buffer = malloc(20 * sizeof(char));
//     sprintf(buffer, ptrStr, exception);
//     printf("exception: %s\n", buffer);
//     return &buffer;
// }

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
    printf("exception: %s\n", buffer);

    string exceptionStr(buffer);

    jdata["exception"] = (char*)exceptionStr.c_str();


    char *methodName;
    err = jvmtiEnv->GetMethodName(method,
                        &methodName, NULL, NULL);

    // record calling method
    jdata["callingMethod"] = (char * )methodName;

    if(backTraceEnabled){ // only run if the backtrace is enabled
        int numFrames = 5;
        jvmtiFrameInfo frames[numFrames];
        jint count;
        jvmtiError err;
        auto jMethods = json::array();

        err = jvmtiEnv->GetStackTrace(thread, 0, numFrames,
                                    frames, &count);
        if (err == JVMTI_ERROR_NONE && count >= 1) {
            char *methodName;
            json jMethod;
            for (int i = 0; i < count; i++) {
                err = jvmtiEnv->GetMethodName(frames[i].method,
                                    &methodName, NULL, NULL);
                if (err == JVMTI_ERROR_NONE) {
                    jMethod["methodName"] = methodName;
                    jMethods.push_back(jMethod);
                }
            }
        }

        jdata["backtrace"] = jMethods;
    }

    jdata["numExceptions"] = numExceptions;

    sendToServer(jdata.dump());
}
