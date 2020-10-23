#include <jvmti.h>
#include <string.h>
#include "objectalloc.h"
#include "AgentOptions.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.h"

using json = nlohmann::json;

JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv, 
                        JNIEnv* env, 
                        jthread thread, 
                        jobject object, 
                        jclass object_klass, 
                        jlong size) {
    jvmtiError err;
    // jobject cls = env->GetObjectClass(object);
    // jobjectRefType clsType = env->GetObjectRefType(object_klass);
    json j;

    // err = jvmtiEnv->GetClassSignature(object_klass, &refType, NULL);
    // if (err == JVMTI_ERROR_NONE) {
    //     j["classType"] = refType;
    //     std::string s = j.dump();
    //     printf("\n%s\n", s.c_str());
    // }

    jint class_count;
    jclass* classes;

    err = jvmtiEnv->GetLoadedClasses(&class_count, &classes);
    if (err == JVMTI_ERROR_NONE && class_count >= 1) {
        for (int i = 0; i < class_count; i++) {
            char * refType;
            err = jvmtiEnv->GetClassSignature(classes[i], &refType, NULL);
            if (err == JVMTI_ERROR_NONE) {
                j["refNum"] = i;
                j["refType"] = refType;
            }
        }
    }

    // jint * hcode;
    // err = jvmtiEnv->GetObjectHashCode(object, &hcode);
    // if (err == JVMTI_ERROR_NONE) {
    //     j["address"] = hcode;
    // }
    
    std::string s = j.dump();
    printf("\n%s\n", s.c_str());
    // sendMessageToClients(j.dump());
                            

}