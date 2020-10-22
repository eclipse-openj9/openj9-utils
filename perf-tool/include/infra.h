#ifndef INFRA_H_
#define INFRA_H_

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread);
JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env);

extern int portNo;


#endif /* INFRA_H_ */