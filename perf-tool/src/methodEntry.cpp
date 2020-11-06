#include <jvmti.h>
#include <string.h>
#include "agentOptions.hpp"
#include "methodEntry.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.hpp"

#include <iostream>

using json = nlohmann::json;

// std::atomic<int> sampleCount {0};

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread,
            jmethodID method) {


    json j;
    jvmtiError err;
    char *name_ptr;
    char *signature_ptr;
    char *generic_ptr;

    printf("\nmethodEntryEvents entered");

    err = jvmtiEnv->GetMethodName(method, &name_ptr, &signature_ptr, &generic_ptr);

    check_jvmti_error(jvmtiEnv, err, "Unable to get the method name\n");

    if (err == JVMTI_ERROR_NONE) {
        j["methodName"] = name_ptr;
        j["methodSig"] = signature_ptr;
    }

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
                }
            }
        }
    }
    j["methodBacktrace"] = jMethods;

    std::string s = j.dump();
    printf("\n%s\n", s.c_str());
    sendToServer(s);
}