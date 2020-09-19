#ifndef INFRA_H_
#define INFRA_H_

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread);
JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env);

#endif /* INFRA_H_ */