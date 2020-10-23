#include <jvmti.h>
#include <string.h>
#include "AgentOptions.hpp"
#include "methodentry.h"
#include "json.hpp"
#include "server.hpp"
#include "infra.h"

#include <iostream>

using json = nlohmann::json;

JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
            JNIEnv* env,
            jthread thread,
            jmethodID method) {

    /*jvmtiError err;
    json j;
    char *methodName;
    err = jvmtiEnv->GetMethodName(method, &methodName, NULL, NULL);
    if (err == JVMTI_ERROR_NONE && strcmp(methodName, "doBatch") == 0) {
        j["methodName"] = methodName;
        std::string s = j.dump();

        printf("\n%s\n", s.c_str());
    }*/

    json j;
    jvmtiError err;
    char *name_ptr;
    char *signature_ptr;
    char *generic_ptr;

    err = jvmtiEnv->GetMethodName(method, &name_ptr, &signature_ptr, &generic_ptr);

    check_jvmti_error(jvmtiEnv, err, "Unable to get the method name");

    if (err == JVMTI_ERROR_NONE) {
        printf("Entered method %s\n", name_ptr);
    }

    // std::string s = j.dump();
    // printf("\n%s\n", s.c_str());
    // sendMessageToClients(s);
}