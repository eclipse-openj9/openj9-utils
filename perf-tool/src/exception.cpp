#include <atomic>
#include <jvmti.h>
#include <string>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"
#include "exception.hpp"

using namespace std;
using json = nlohmann::json;

std::atomic<bool> backTraceEnabled;
std::atomic<int> exceptionSampleCount {0};
std::atomic<int> exceptionSampleRate {1};


void setExceptionSampleRate(int rate) {
    // Set sampling rate and enable/disable backtrace
    if (rate > 0) {
        setExceptionBackTrace(true);
        exceptionSampleRate = rate;
    } else {
        setExceptionBackTrace(false);
    }
}


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

    string exceptionStr(buffer);

    jdata["exception"] = (char*)exceptionStr.c_str();


    char *methodName;
    err = jvmtiEnv->GetMethodName(method,
                        &methodName, NULL, NULL);

    // record calling method
    jdata["callingMethod"] = (char * )methodName;

    if (backTraceEnabled) { // only run when backtrace is enabled

        if (exceptionSampleCount % exceptionSampleRate == 0) {
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
                    /* Get method name */
                    err = jvmtiEnv->GetMethodName(frames[i].method,
                                        &methodName, NULL, NULL);
                    if (err == JVMTI_ERROR_NONE) {
                        jMethod["methodName"] = methodName;
                    }

                    /* Get method line table */
                    int lineCount, lineNumber;
                    jvmtiLineNumberEntry *lineTable;

                    err = jvmtiEnv->GetLineNumberTable(frames[i].method, &lineCount, &lineTable);
                    if ( err == JVMTI_ERROR_NONE ) { // Find line
                        lineNumber = lineTable[0].line_number;
                        for (int j = 1; j < lineCount; j++) {
                            if (frames[i].location < lineTable[i].start_location) {
                                break;
                            } else {
                                lineNumber = lineTable[i].line_number;
                            }
                        }
                        jMethod["lineNumber"] = lineNumber;
                    }

                    /*TO-DO: Get file name*/



                    jMethods.push_back(jMethod);
                }
            }

            jdata["backtrace"] = jMethods;
        }

        exceptionSampleCount++;
    }

    jdata["numExceptions"] = numExceptions;

    sendToServer(jdata.dump());
}
