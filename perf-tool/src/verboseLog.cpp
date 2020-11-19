#include "verboseLog.hpp"

#include <iostream>

#include <jvmti.h>

#include "agentOptions.hpp"
#include "infra.hpp"

using namespace std;

void VerboseLogSubscriber::Subscribe()
{
    jvmtiError rc;
    jvmtiVerboseGCSubscriber subscriberCallback =  &verboseSubscriberCallback;
    jvmtiVerboseGCAlarm alarmCallback = &verboseAlarmCallback;
    jint extensionFunctionCount = 0;
    jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
    int i = 0, j = 0;

    /* Look up all the JVMTI extension functions */
    jvmti_env->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);

    /* Find the JVMTI extension function we want */
    while (j++ < extensionFunctionCount)
    {
        jvmtiExtensionFunction function = extensionFunctions->func;

        if (strcmp(extensionFunctions->id, COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER) == 0)
        {
            // Found the register verbose gc subscriber function 
            rc = function(jvmti_env, "verbose log subscriber", subscriberCallback, alarmCallback, NULL, &subscriptionID);
            if (rc != JVMTI_ERROR_NONE)
            {
                perror("ERROR registering verbose log subscriber failed: ");
            }
            printf("Calling JVMTI extension %s, rc=%i\n", COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER, rc);
            break;
        }
        extensionFunctions++; /* move on to the next extension function */
    }
}


void VerboseLogSubscriber::Unsubscribe()
{
    jvmtiError rc;
    jint extensionFunctionCount = 0;
    jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
    int i = 0, j = 0;

    if (subscriptionID == NULL)
    {
        return;
    }

    /* Look up all the JVMTI extension functions */
    jvmti_env->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);

    /* Find the JVMTI extension function we want */
    while (j++ < extensionFunctionCount)
    {
        jvmtiExtensionFunction function = extensionFunctions->func;

        if (strcmp(extensionFunctions->id, COM_IBM_DEREGISTER_VERBOSEGC_SUBSCRIBER) == 0)
        {
            // Found the deregister verbose gc subscriber function 
            rc = function(jvmti_env, NULL, &subscriptionID);
            if (rc != JVMTI_ERROR_NONE)
            {
                perror("ERROR deregistering verbose log subscriber failed: ");
            }
            printf("Calling JVMTI extension %s, rc=%i\n", COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER, rc);
            break;
        }
        extensionFunctions++; /* move on to the next extension function */
    }
}

jvmtiError verboseSubscriberCallback(jvmtiEnv *jvmti_env, const char *record, jlong length, void *user_data)
{
    string s = string(record);
    sendToServerQueue(s);

    return JVMTI_ERROR_NONE;
}

void verboseAlarmCallback(jvmtiEnv *jvmti_env, void *subscription_id, void *user_data)
{
    string s = string("ERROR subscriber returned error");
    sendToServer(s);
}
