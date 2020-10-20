//TODO: figure out objects reachable?

#include <jvmti.h>
#include <string.h>


#include "objectalloc.h"
#include "AgentOptions.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.h"

#include <iostream>
#include <chrono>
#include <ctime>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

using json = nlohmann::json;
using namespace std::chrono;

// JVMTI callback function
static jvmtiIterationControl JNICALL
        reference_object(jvmtiObjectReferenceKind reference_kind,   
        jlong class_tag, jlong size, jlong* tag_ptr,
        jlong referrer_tag, jint referrer_index, void *user_data) {
    return JVMTI_ITERATION_CONTINUE;
}

JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv, 
                        JNIEnv* env, 
                        jthread thread, 
                        jobject object, 
                        jclass object_klass, 
                        jlong size) {
    jvmtiError err;
    json jObj;

    jint class_count;
    jclass* classes;

    char *className;
    

    auto start = steady_clock::now();

    /*** get information about object ***/
    err = jvmtiEnv->GetClassSignature(object_klass, &className, NULL);
    if (className != NULL && err == JVMTI_ERROR_NONE) {
        jObj["objName"] = className;
        jObj["objSizeInBytes"] = (jint)size;
    }

    char *methodName;
    char *methodSignature;
    char *declaringClassName;
    jclass declaring_class;

    jint entry_count_ptr;
    jvmtiLineNumberEntry* table_ptr;
    jlocation start_loc_ptr;
    jlocation end_loc_ptr;

    // print stack trace
    int numFrames = 10;
    jvmtiFrameInfo frames[numFrames];
    jint count;
    int i;


    /*** get information about backtrace at object allocation sites ***/
    // int sze = 0;
    json jBackTrace;
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
                    // printf("at method %s in class %s\n", methodName, declaringClassName);
                    // printf("method signature: %s\n", methodSignature);
                    
                    // printf("entry_count_ptr: %i\n", entry_count_ptr);
                    /*
                    if (err == JVMTI_ERROR_NONE) {
                        // printf("line number %i\n", table_ptr->line_number);
                        jMethod["lineNum"] = table_ptr->line_number;
                    } else
                        printf("err: %d\n", err);
                        printf("couldn't fetch line num for %s \n", methodName);
                    // char num = i;
                    // jObj.emplace("className" + num, declaringClassName);
                    // jObj.emplace("classSize" + num, sizeof(declaring_class));
                    // sze += sizeof(declaring_class);
                    */
                }
            }
            jBackTrace["method "] = jMethod;
        }
    } 
    jObj["objBackTrace"] = jBackTrace;

    // jObj["combinedDeclaredSize"] = sze;
    

    // err = jvmtiEnv->Deallocate((void*)className);
    // err = jvmtiEnv->Deallocate((void*)methodName);
    // err = jvmtiEnv->Deallocate((void*)declaringClassName);


    // err = jvmtiEnv->IterateOverObjectsReachableFromObject (object, &reference_object, NULL);
    // if (err != JVMTI_ERROR_NONE) {
    //     printf("Cannot iterate over reachable objects\n");
    // } else {
    //     printf("iterated reachable objects\n");
    // }

    /*** calculate time taken in microseconds ***/
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    float rate = (float)size/duration;
    // std::cout
    //   << "objectalloc took "
    //   << duration << "microsec ≈ \n"
    //   << "rate: " << rate << "bytes/microsec \n"
    //   ;
    jObj["objAllocRateInBytesPerMicrosec"] = rate;
    
    json j;
    j["object"] = jObj; 


    std::string s = j.dump();
    printf("\n%s\n", s.c_str());
    // sendMessageToClients(j.dump());
                            



}
