#include <jvmti.h>
#include <string.h>
#include "agentOptions.hpp"
#include "methodEntry.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.hpp"

#include <iostream>

using json = nlohmann::json;

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread,
            jmethodID method) {


    json j;
    jvmtiError err;
    char *name_ptr;
    char *signature_ptr;
    char *generic_ptr;

    err = jvmtiEnv->GetMethodName(method, &name_ptr, &signature_ptr, &generic_ptr);

    check_jvmti_error(jvmtiEnv, err, "Unable to get the method name\n");

    if (err == JVMTI_ERROR_NONE) {
        j["methodName"] = name_ptr;
        j["methodSig"] = signature_ptr;
    }

    std::string s = j.dump();
    printf("\n%s\n", s.c_str());
    sendToServer(s);
}