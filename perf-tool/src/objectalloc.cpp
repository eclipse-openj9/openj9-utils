// TODO: object count
#include <jvmti.h>
#include <string.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <atomic>

using json = nlohmann::json;
using namespace std::chrono;

std::atomic<bool> objAllocBackTraceEnabled {true};
std::atomic<int> objAllocSampleCount {0};
std::atomic<int> objAllocSampleRate {1};

// Enables or disables the back trace option
void setObjAllocBackTrace(bool val){
    objAllocBackTraceEnabled = val;
    return;
}

// to turn off backTrace, set sampleRate to 0
void setObjAllocSampleRate(int rate) {
    if (rate > 0) {
        objAllocBackTraceEnabled = true;
        objAllocSampleRate = rate;
    } else {
        objAllocBackTraceEnabled = false;
    }
    return;
}

JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv, 
                        JNIEnv* env, 
                        jthread thread, 
                        jobject object, 
                        jclass object_klass, 
                        jlong size) {
    jvmtiError err;

    json jObj;
    char *classType;
    auto start = steady_clock::now();


    /*** get information about object ***/
    err = jvmtiEnv->GetClassSignature(object_klass, &classType, NULL);
    if (classType != NULL && err == JVMTI_ERROR_NONE) {
        jObj["objType"] = classType;
        jObj["size"] = (jint)size;
    }


    /*** get information about backtrace at object allocation sites if enabled***/
    if (objAllocBackTraceEnabled) {
        if (objAllocSampleCount % objAllocSampleRate == 0){
            char *methodName;
            char *methodSignature;
            char *declaringClassName;
            jclass declaring_class;
            jint entry_count_ptr;
            jvmtiLineNumberEntry* table_ptr;
            jlocation start_loc_ptr;
            jlocation end_loc_ptr;
            int numFrames = 10;
            jvmtiFrameInfo frames[numFrames];
            jint count;
            int i;
            // int sze = 0;
            auto jMethods = json::array();
            err = jvmtiEnv->GetStackTrace(NULL, 0, numFrames, frames, &count);
            if (err == JVMTI_ERROR_NONE && count >= 1) {
                for (i = 0; i < count; i++) {
                    json jMethod;
                    err = jvmtiEnv->GetMethodName(frames[i].method, &methodName, &methodSignature, NULL);
                    if (err == JVMTI_ERROR_NONE) {
                        err = jvmtiEnv->GetMethodDeclaringClass(frames[i].method, &declaring_class);
                        err = jvmtiEnv->GetMethodLocation(frames[i].method, &start_loc_ptr, &end_loc_ptr);
                        err = jvmtiEnv->GetClassSignature(declaring_class, &declaringClassName, NULL);
                        err = jvmtiEnv->GetLineNumberTable(frames[i].method, &entry_count_ptr, &table_ptr);
                        if (err == JVMTI_ERROR_NONE) {
                            jMethod["methodName"] = methodName;
                            jMethod["methodSignature"] = methodSignature;
                            jMethod["lineNum"] = table_ptr->line_number;
                            jMethod["methodNum"] = i;
                            jMethods.push_back(jMethod);
                        }
                    }
                }
            } 
                
            err = jvmtiEnv->Deallocate((unsigned char*)methodSignature);
            err = jvmtiEnv->Deallocate((unsigned char*)methodName);
            err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);

            jObj["objBackTrace"] = jMethods;
            jObj["objBackTraceSampleNum"] = objAllocSampleCount.load();
        }
        objAllocSampleCount++;
    }

    /*** calculate time taken in microseconds and calculate rate ***/
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    float rate = (float)size/duration;
    jObj["objAllocRate"] = rate;

    json j;
    j["object"] = jObj; 
    std::string s = j.dump();
    // printf("\n%s\n", s.c_str());
    sendToServer(s);

}