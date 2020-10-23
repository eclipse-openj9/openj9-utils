#include <jvmti.h>
#include <string.h>
#include "objectalloc.h"
#include "AgentOptions.hpp"
#include "json.hpp"
#include "server.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

#include "infra.h"

using json = nlohmann::json;
using namespace std::chrono;

JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv, 
                        JNIEnv* env, 
                        jthread thread, 
                        jobject object, 
                        jclass object_klass, 
                        jlong size) {
    jvmtiError err;
    json jObj;
    char *className;
    auto start = steady_clock::now();

/****************************************************************/
    // jclass cls = env->GetObjectClass(object);
    // // First get the class object
    // jmethodID mid = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");
    // jobject clsObj = env->CallObjectMethod(object, mid);
    // // Now get the class object's class descriptor
    // cls = env->GetObjectClass(clsObj);
    // // Find the getName() method on the class object
    // mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    // // Call the getName() to get a jstring object back
    // jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);
    // // Now get the c string from the java jstring object
    // const char* str = env->GetStringUTFChars(strObj, NULL);
    // // record calling class
    // jObj["objName"] = str;
    // // Release the memory pinned char array
    // env->ReleaseStringUTFChars(strObj, str);
    // jObj["objSizeInBytes"] = (jint)size;
/****************************************************************/

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
                    jMethods.push_back(jMethod);
                    // printf("at method %s in class %s\n", methodName, declaringClassName);
                    // printf("method signature: %s\n", methodSignature);
                    // printf("line number %i\n", table_ptr->line_number);
                    // sze += sizeof(declaring_class);
                }
            }
        }
    } 
    jObj["objBackTrace"] = jMethods;
    // jObj["combinedDeclaredSize"] = sze;

    // err = jvmtiEnv->Deallocate((void*)className);
    // err = jvmtiEnv->Deallocate((void*)methodName);
    // err = jvmtiEnv->Deallocate((void*)declaringClassName);
    
    /*** calculate time taken in microseconds ***/
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    float rate = (float)size/duration;
    jObj["objAllocRateInBytesPerMicrosec"] = rate;
    json j;
    j["object"] = jObj; 

    std::string s = j.dump();
    // sendToServer(s);
}