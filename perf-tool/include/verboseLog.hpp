#ifndef VERBOSE_LOG_H_
#define VERBOSE_LOG_H_

void StartVerboseLogCollector(jvmtiEnv *jvmti_env);
jvmtiError verboseLogSubscriber(jvmtiEnv *env, const char *record, jlong length, void *userData);

#endif