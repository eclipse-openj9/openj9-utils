#include <jvmti.h>
#include <string.h>
#include "agentOptions.hpp"
#include "methodEntry.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.hpp"

#include <iostream>
#include <atomic>

using json = nlohmann::json;

std::atomic<int> mEntrySampleCount {0};
std::atomic<int> mEntrySampleRate {1};

// set sample rate according to command instructions
// requirement: rate > 0
void setMethodEntrySampleRate(int rate) {
    mEntrySampleRate = rate;
}

/*** retrieves method name and line number, and declaring class name and signature
 *      for every nth method entry                                                 ***/
JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread,
            jmethodID method) {

    if (mEntrySampleCount % mEntrySampleRate == 0) {           

        json j;
        jvmtiError err;
        char *name_ptr;
        char *signature_ptr;
        char *generic_ptr;
        char *declaringClassName;
        jclass declaring_class;
        jint entry_count_ptr;
        jvmtiLineNumberEntry* table_ptr;
        jlocation start_loc_ptr;
        jlocation end_loc_ptr;

        err = jvmtiEnv->GetMethodName(method, &name_ptr, &signature_ptr, &generic_ptr);
        err = jvmtiEnv->GetMethodDeclaringClass(method, &declaring_class);
        err = jvmtiEnv->GetClassSignature(declaring_class, &declaringClassName, NULL);
        err = jvmtiEnv->GetMethodLocation(method, &start_loc_ptr, &end_loc_ptr);
        err = jvmtiEnv->GetLineNumberTable(method, &entry_count_ptr, &table_ptr);

        if (err == JVMTI_ERROR_NONE) {
            j["methodName"] = name_ptr;
            j["methodSig"] = signature_ptr;
            j["methodClass"] = declaringClassName;
            j["methodLineNum"] = table_ptr->line_number;
            j["methodNum"] = mEntrySampleCount.load();
        }

        err = jvmtiEnv->Deallocate((unsigned char*)name_ptr);
        err = jvmtiEnv->Deallocate((unsigned char*)signature_ptr);
        err = jvmtiEnv->Deallocate((unsigned char*)generic_ptr);
        err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);

    
        std::string s = j.dump();
        // printf("\n%s\n", s.c_str());
        // printf("sample num: %i", mEntrySampleCount);
        sendToServer(s);
    }
    mEntrySampleCount++;
}
