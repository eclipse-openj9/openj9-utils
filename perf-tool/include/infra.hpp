#ifndef INFRA_H_
#define INFRA_H_

#include <string>
#include <jvmti.h>

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread);
JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env);
void JNICALL startServer(jvmtiEnv * jvmti, JNIEnv* jni);
void sendToServer(std::string message);

extern int portNo;
extern std::string commandsPath;
extern std::string logPath;


#endif /* INFRA_H_ */