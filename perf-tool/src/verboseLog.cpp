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

void VerboseLogSubscriber::setVerboseGCLogSampleRate(int rate) 
{
    if (rate > 0)
        verboseSampleRate = rate;
}

void VerboseLogSubscriber::Subscribe()
{
    if (!ExtensionFunctions::_verboseGCSubcriber)
    {
        if (verbose >= Verbose::ERROR)
            fprintf(stderr, "Error using GC subscriber: OpenJ9 JVMTI extensions not found\n");
        return;
    }
    jvmtiVerboseGCSubscriber subscriberCallback =  &verboseSubscriberCallback;
    jvmtiVerboseGCAlarm alarmCallback = &verboseAlarmCallback;

    if (verbose >= Verbose::INFO)
        printf("Subscribe to verbose GC\n");

    // Use the register verbose gc subscriber function
    jvmtiError rc = (ExtensionFunctions::_verboseGCSubcriber)(jvmti_env, "verbose log subscriber", subscriberCallback, alarmCallback, NULL, &subscriptionID);
    check_jvmti_error(jvmti_env, rc, "registering verbose log subscriber failed");
}


void VerboseLogSubscriber::Unsubscribe()
{
    if (!ExtensionFunctions::_verboseGCUnsubcriber)
    {
        if (verbose >= Verbose::ERROR)
            fprintf(stderr, "Error using GC unsubscriber: OpenJ9 JVMTI extensions not found\n");
        return;
    }
    if (subscriptionID == NULL)
    {
        if (verbose >= Verbose::WARN)
            fprintf(stderr, "Aborting verbose GC unsubscription because subscriptionID is NULL\n");
        return;
    }
    if (verbose >= Verbose::INFO)
        printf("Unsubscribe to verbose GC\n");

    jvmtiError rc = (ExtensionFunctions::_verboseGCUnsubcriber)(jvmti_env, NULL, &subscriptionID);
    check_jvmti_error(jvmti_env, rc, "de-registering verbose log subscriber failed");
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
