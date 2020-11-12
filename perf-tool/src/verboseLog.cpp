#include <iostream>
#include <jvmti.h>
#include <ibmjvmti.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"
#include "verboseLog.hpp"

using json = nlohmann::json;

using namespace std;

void StartVerboseLogCollector(jvmtiEnv *jvmti_env)
{
    jvmtiError rc;
    // jint extensionEventCount = 0;
    // jvmtiExtensionEventInfo *extensionEvents = NULL;
    jvmtiVerboseGCSubscriber subscriberCallback = &verboseLogSubscriber;
    jvmtiVerboseGCAlarm alarmCallback = &verboseAlarm;
    jint extensionFunctionCount = 0;
    jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
    int i = 0, j = 0;

    /* Look up all the JVMTI extension events and functions */
    // jvmti->GetExtensionEvents(&extensionEventCount, &extensionEvents);
    jvmti_env->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);

    /* Find the JVMTI extension event we want */
    // while (i++ < extensionEventCount)
    // {

    //     if (strcmp(extensionEvents->id, COM_IBM_QUERY_VM_LOG_OPTIONS) == 0)
    //     {
    //         /* Found the dump start extension event, now set up a callback for it */
    //         rc = jvmti->SetExtensionEventCallback(extensionEvents->extension_event_index, &DumpStartCallback);
    //         printf("Setting JVMTI event callback %s, rc=%i\n", COM_IBM_VM_DUMP_START, rc);
    //         break;
    //     }
    //     extensionEvents++; /* move on to the next extension event */
    // }

    /* Find the JVMTI extension function we want */
    while (j++ < extensionFunctionCount)
    {
        jvmtiExtensionFunction function = extensionFunctions->func;

        if (strcmp(extensionFunctions->id, COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER) == 0)
        {
            /* Found the set dump extension function, now set a dump option to generate javadumps on
            thread starts */
            cout << "REACHED HERE 1" << endl;
            rc = function(jvmti_env, "", subscriberCallback, alarmCallback, NULL, NULL, 1);
            cout << "REACHED HERE 2" << endl;
            printf("Calling JVMTI extension %s, rc=%i\n", COM_IBM_SET_VM_DUMP, rc);
            break;
        }
        extensionFunctions++; /* move on to the next extension function */
    }
}

jvmtiError verboseLogSubscriber(jvmtiEnv *env, const char *record, jlong length, void *userData)
{
    string s = string(record);
    sendToServer(s);

    return JVMTI_ERROR_NONE;
}

jvmtiError verboseAlarm(jvmtiEnv *env, const char *record, jlong length, void *userData)
{
    string s = string(record);
    sendToServer(s);

    return JVMTI_ERROR_NONE;
}