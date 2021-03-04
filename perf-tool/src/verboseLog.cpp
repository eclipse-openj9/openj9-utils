/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "verboseLog.hpp"

#include <iostream>
#include <jvmti.h>

#include "agentOptions.hpp"
#include "infra.hpp"

using namespace std;

std::atomic<int> verboseSampleCount {0};
std::atomic<int> verboseSampleRate {1};

void VerboseLogSubscriber::setVerboseGCLogSampleRate(int rate) {
    if (rate > 0) {
        verboseSampleRate = rate;
    }
}

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
            if (verbose >= WARN)
                printf("Calling JVMTI extension %s, rc=%i\n", COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER, rc);
            break;
        }
        extensionFunctions++; /* move on to the next extension function */
    }
}

jvmtiError verboseSubscriberCallback(jvmtiEnv *jvmti_env, const char *record, jlong length, void *user_data)
{
    if (verboseSampleCount % verboseSampleRate == 0)
    {
        string s = string(record);
        sendToServer(s);
    }

    verboseSampleCount++;

    return JVMTI_ERROR_NONE;
}

void verboseAlarmCallback(jvmtiEnv *jvmti_env, void *subscription_id, void *user_data)
{
    string s = string("ERROR subscriber returned error");
    sendToServer(s);
}
