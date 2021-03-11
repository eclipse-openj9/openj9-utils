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

#include <jvmti.h>
#include <ibmjvmti.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "monitor.hpp"
#include "objectalloc.hpp"
#include "exception.hpp"
#include "methodEntry.hpp"
#include "verboseLog.hpp"

#include "json.hpp"

using json = nlohmann::json;

VerboseLogSubscriber *verboseLogSubscriber;
extern EventConfig monitorConfig;

void invalidCommand(const std::string& function, const std::string& command)
{
    if (verbose >= ERROR)	
        fprintf(stderr, "Invalid command with parameters: {functionality: %s, command: %s}\n", function.c_str(), command.c_str());
}

void invalidFunction(const std::string& function, const std::string& command)
{
    if (verbose >= ERROR)
        fprintf(stderr, "Invalid function with parameters: {functionality: %s, command: %s}\n", function.c_str(), command.c_str());
}

void invalidRate(const std::string& function, const std::string& command, int sampleRate)
{
    if (verbose >= ERROR)
        fprintf(stderr, "Invalid rate with parameters: {functionality: %s, command: %s, sampleRate: %i}\n", function.c_str(), command.c_str(), sampleRate);
}

void invalidStackTraceDepth(const std::string& function, const std::string& command, int stackTraceDepth)
{
    if (verbose >= ERROR)
        fprintf(stderr, "Invalid stackTraceDepth with parameters: {functionality: %s, command: %s, stackTraceDepth: %i}\n", function.c_str(), command.c_str(), stackTraceDepth);
}

void modifyMonitorEvents(const std::string& function, const std::string& command, int rate, int stackTraceDepth,
                         const std::string& callbackClass, const std::string& callbackMethod, const std::string& callbackSignature)
{
    jvmtiCapabilities capa;
    jvmtiError error;

    memset(&capa, 0, sizeof(jvmtiCapabilities));
    monitorConfig.setSampleRate(rate);
    monitorConfig.setStackTraceDepth(stackTraceDepth);
    monitorConfig.setCallbacks(callbackClass, callbackMethod, callbackSignature);
    if (!command.compare("start"))
    {
        error = jvmti->GetCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to get current capabilties.");
        if (!capa.can_generate_monitor_events)
        {
            capa.can_generate_monitor_events = 1;
        }
        if (!capa.can_get_monitor_info)
        {
            capa.can_get_monitor_info = 1;
        }
        error = jvmti->AddCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to init Monitor Events capability.");
        error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable MonitorContendedEntered event notifications.");
        error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable MonitorContendedEnter event notifications.");
    }
    else if (!command.compare("stop"))
    {
        error = jvmti->GetCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to get current capabilties.");
        error = jvmti->RelinquishCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to relinquish Monitor Events Capability.");
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.");
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEnter event.");
    } 
    else 
    {
        invalidCommand(function, command);
    }
    return;
}

void modifyObjectAllocEvents(const std::string& function,const std::string& command, int sampleRate)
{
    jvmtiCapabilities capa;
    jvmtiError error;

    memset(&capa, 0, sizeof(jvmtiCapabilities));
    error = jvmti->GetCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Unable to get current capabilties.");
    setObjAllocSampleRate(sampleRate);
    if (capa.can_generate_vm_object_alloc_events)
    {
        if (!command.compare("stop"))
        {
            memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_vm_object_alloc_events = 1;

            error = jvmti->RelinquishCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to relinquish object alloc capability.");

            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable ObjectAlloc event.");
        }
        else if (!command.compare("start"))
        { 
            printf("Object Alloc Capability already enabled.");
            error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable VM ObjectAlloc event notifications.");
        }
        else
        {
            invalidCommand(function, command);
        }
    }
    else
    { /* cannot generate object alloc events */
        if (!command.compare("start"))
        {
            memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_vm_object_alloc_events = 1;

            error = jvmti->AddCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to init object alloc events capability");

            error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable VM ObjectAlloc event notifications.");
        }
        else if (!command.compare("stop"))
        {
            if (verbose == INFO)
                printf("Obect Alloc Capability already disabled");
            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable ObjectAlloc event.");
        }
        else
        {
            invalidCommand(function, command);
        }
    }
}

void modifyMonitorStackTrace(const std::string& function, const std::string& command)
{
    /* enable stack trace */
    if (!command.compare("start"))
    {
        setMonitorStackTrace(true);
    }
    else if (!command.compare("stop"))
    {
        setMonitorStackTrace(false);
    }
    else
    {
        invalidCommand(function, command);
    }
}

void modifyMethodEntryEvents(const std::string& function, const std::string& command, int sampleRate)
{
    jvmtiError error;
    setMethodEntrySampleRate(sampleRate);
    if (!command.compare("stop"))
    {
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable MethodEntry event.");
    }
    else if (!command.compare("start"))
    { 
        error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable MethodEntry event notifications.");
    }
    else
    {
        invalidCommand(function, command);
    }
}

void modifyVerboseLogSubscriber(const std::string& function, const std::string& command, int sampleRate)
{
    /* enable stack trace */
    if (!command.compare("start"))
    {
        verboseLogSubscriber = new VerboseLogSubscriber(jvmti);
        verboseLogSubscriber->setVerboseGCLogSampleRate(sampleRate);
        verboseLogSubscriber->Subscribe();
    } else if (!command.compare("stop"))
    {
        verboseLogSubscriber->Unsubscribe();
        delete verboseLogSubscriber;
    } else {
        invalidCommand(function, command);
    }
}

void modifyExceptionBackTrace(const std::string& function, const std::string& command)
{
    /* enable stack trace */
    if (!command.compare("start"))
    {
        setExceptionBackTrace(true);
    }
    else if (!command.compare("stop"))
    {
        setExceptionBackTrace(false);
    }
    else
    {
        invalidCommand(function, command);
    }
}

void modifyExceptionEvents(const std::string& function, const std::string& command, int sampleRate)
{
    jvmtiError error;
    setExceptionSampleRate(sampleRate);

    if (!command.compare("start"))
    {
        error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable Exception event notifications.");
    }
    else if (!command.compare("stop"))
    {
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable Exception event.");
    }
    else
    {
        invalidCommand(function, command);
    }
}


void agentCommand(const json& jCommand)
{
    jvmtiCapabilities capa;
    jvmtiError error;
    jvmtiPhase phase;

    std::string function = jCommand["functionality"].get<std::string>();
    std::string command  = jCommand["command"].get<std::string>();
    int sampleRate = 1; /* sampleRate is automatically set to 1. To turn off, set to 0 */
    if (jCommand.contains("sampleRate"))
    {
        sampleRate = jCommand["sampleRate"].get<int>();
        if (sampleRate < 0)
        {
            invalidRate(function, command, sampleRate);
        }
    }
    int stackTraceDepth = 0; /* default is no stacktrace */
    if (jCommand.contains("stackTraceDepth"))
    {
        stackTraceDepth = jCommand["stackTraceDepth"].get<int>();
        if (stackTraceDepth < 0)
        {
            invalidStackTraceDepth(function, command, stackTraceDepth);
        }
    }
    std::string callbackClass;
    std::string callbackMethod;
    std::string callbackSignature;
    if (jCommand.contains("callback"))
        {
        auto jcallback = jCommand["callback"];
        callbackClass = jcallback["class"].get<std::string>();
        callbackMethod = jcallback["method"].get<std::string>();
        callbackSignature = jcallback["signature"].get<std::string>();
        if (verbose == INFO)
            printf("Found callback: %s.%s%s\n", callbackClass.c_str(), callbackMethod.c_str(), callbackSignature.c_str());
        }
 

    jvmti->GetPhase(&phase);
    if (!(phase == JVMTI_PHASE_ONLOAD || phase == JVMTI_PHASE_LIVE))
    {
        check_jvmti_error(jvmti, JVMTI_ERROR_WRONG_PHASE, "AGENT CANNOT RECEIVE COMMANDS DURING THIS PHASE!");
    }
    else
    {
        if (!function.compare("monitorEvents"))
        {
            modifyMonitorEvents(function, command, sampleRate, stackTraceDepth, callbackClass, callbackMethod, callbackSignature);
        }
        else if (!function.compare("objectAllocEvents"))
        {
            modifyObjectAllocEvents(function, command, sampleRate);
        }
        else if (!function.compare("monitorStackTrace"))
        {
            modifyMonitorStackTrace(function, command);
        }
        else if (!function.compare("methodEntryEvents"))
        {
            modifyMethodEntryEvents(function, command, sampleRate);
        }
        else if (!function.compare("exceptionEvents"))
        {
            modifyExceptionEvents(function, command, sampleRate);
        }
        else if (!function.compare("verboseLog"))
        {
            modifyVerboseLogSubscriber(function, command, sampleRate);
        }
        else
        {
            invalidFunction(function, command);
        }
    }
    return;
}
