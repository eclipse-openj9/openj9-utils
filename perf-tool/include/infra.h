#ifndef INFRA_H_
#define INFRA_H_

#include "server.hpp"

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread);
JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env);

extern int portNo;
extern Server *server;


#endif /* INFRA_H_ */