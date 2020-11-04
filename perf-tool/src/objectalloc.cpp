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
std::atomic<bool> rateEnabled {true};

// Enables or disables the back trace option
void setObjAllocBackTrace(bool val){
    objAllocBackTraceEnabled = val;
    return;
}

// Enables or disables the rate option
void setRate(bool val){
    rateEnabled = val;
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

    // TODO:
    objAllocBackTraceEnabled = 1;
    rateEnabled = 1;

/****************************************************************/
    // // jclass cls = env->GetObjectClass(object);
    // // First get the class object
    // jmethodID mid = env->GetMethodID(object_klass, "getClass", "()Ljava/lang/Class;");
    // jobject clsObj = env->CallObjectMethod(object, mid);
    // std::cout << clsObj << "\n";
    // jboolean flag = env->ExceptionCheck();
    // if (flag) {
    //     // env->ExceptionDescribe();
    //     // env->ExceptionClear();
    //     jboolean isCopy = false;
    //     jmethodID toString = env->GetMethodID(env->FindClass("java/lang/Object"), "toString", "()Ljava/lang/String;");
    //     jthrowable exc = env->ExceptionOccurred();
    //     jstring s = (jstring)env->CallObjectMethod(exc, toString);
    //     const char* utf = env->GetStringUTFChars(s, &isCopy);
    //     printf("error: %s", utf);
    //     /* code to handle exception */
    // }

    // // Now get the class object's class descriptor
    // jclass cls = env->GetObjectClass(clsObj);
    // // Find the getName() method on the class object
    // mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    // // Call the getName() to get a jstring object back
    // jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);
    // // Now get the c string from the java jstring object
    // const char* str = env->GetStringUTFChars(strObj, NULL);
    // // record calling class
    // printf("objType: %s", str);
    // jObj["objType"] = str;
    // // Release the memory pinned char array
    // env->ReleaseStringUTFChars(strObj, str);
    // jObj["objSizeInBytes"] = (jint)size;
/****************************************************************/

    /*** get information about object ***/
    err = jvmtiEnv->GetClassSignature(object_klass, &classType, NULL);
    if (classType != NULL && err == JVMTI_ERROR_NONE) {
        jObj["objType"] = classType;
        jObj["size"] = (jint)size;
    }


    /*** get information about backtrace at object allocation sites if enabled***/
    if (objAllocBackTraceEnabled) {
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
    }

    /*** calculate time taken in microseconds and calculate rate if enabled ***/
    if (rateEnabled) {
        auto end = steady_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        float rate = (float)size/duration;
        jObj["objAllocRateInBytesPerMicrosec"] = rate;
    }

    json j;
    j["object"] = jObj; 
    std::string s = j.dump(2, ' ', true);
    // printf("\n%s\n", s.c_str());
    sendToServer(s);

    // err = jvmtiEnv->Deallocate((unsigned char*)methodSignature);
    // err = jvmtiEnv->Deallocate((unsigned char*)methodName);
    // err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);
}