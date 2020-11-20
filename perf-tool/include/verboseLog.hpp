#ifndef VERBOSE_LOG_H_
#define VERBOSE_LOG_H_

#include <ibmjvmti.h>
#include <atomic>

void verboseAlarmCallback(jvmtiEnv *jvmti_env, void *subscription_id, void *user_data);
jvmtiError verboseSubscriberCallback(jvmtiEnv *jvmti_env, const char *record, jlong length, void *user_data);

class VerboseLogSubscriber
{
    /*
     * Data members
     */
protected:
public:
private:
    jvmtiEnv *jvmti_env;
    void *subscriptionID;

    /*
     * Function members
     */
protected:
public:
    VerboseLogSubscriber(jvmtiEnv *_jvmti_env)
    {
        jvmti_env = _jvmti_env;
    }

    void setVerboseGCLogSampleRate(int rate);
    void Subscribe();
    void Unsubscribe();

private:

};
#endif
