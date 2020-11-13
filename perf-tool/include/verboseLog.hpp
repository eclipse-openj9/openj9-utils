#ifndef VERBOSE_LOG_H_
#define VERBOSE_LOG_H_

#include <ibmjvmti.h>

void StartVerboseLogCollector(jvmtiEnv *jvmti_env);
void verboseLogAlarm(jvmtiEnv *jvmti_env, void *subscription_id, void *user_data);
jvmtiError verboseLogSubscriber(jvmtiEnv *jvmti_env, const char *record, jlong length, void *user_data);

#endif
