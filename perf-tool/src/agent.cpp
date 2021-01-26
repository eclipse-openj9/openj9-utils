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
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"
#include "methodEntry.hpp"
#include "monitor.hpp"
#include "objectalloc.hpp"
#include "server.hpp"
#include "exception.hpp"
#include "serverClients.hpp"

using json = nlohmann::json;

jvmtiEnv *jvmti;

/* Server arguments with defaults */
int portNo = ServerConstants::DEFAULT_PORT;
std::string commandsPath = "";
std::string logPath = "logs.json";

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    std::string token;
    std::string optionsDelim = ",";
    std::string pathDelim = ":";
    std::string oIn = (std::string)options;
    int pos1, pos2 = 0;

    /* there is a max of two options the user can supply here
     * "commands" is followed by a path to the commands file
     * "log" is followed by the path to the location for the log file
     * "portno" is followed by a number indicating the port to run the server on 
     */
    while ((pos1 = oIn.find(optionsDelim)) != std::string::npos)
    {
        if (pos1 != std::string::npos)
        {
            token = oIn.substr(0, pos1);
            if ((pos2 = token.find(pathDelim)) != std::string::npos)
            {
                if (!token.substr(0, pos2).compare("commandFile"))
                {
                    token.erase(0, pos2 + pathDelim.length());
                    commandsPath = token;
                }
                else if (!token.substr(0, pos2).compare("logFile"))
                {
                    token.erase(0, pos2 + pathDelim.length());
                    logPath = token;
                }
                else if (!token.substr(0, pos2).compare("portNo"))
                {
                    token.erase(0, pos2 + pathDelim.length());
                    portNo = stoi(token);
                }
            }
            oIn.erase(0, pos1 + optionsDelim.length());
        }
    }
    if (!oIn.empty())
    {
        token = oIn;
        if ((pos2 = token.find(pathDelim)) != std::string::npos)
        {
            if (!token.substr(0, pos2).compare("commandFile"))
            {
                token.erase(0, pos2 + pathDelim.length());
                commandsPath = token;
            }
            else if (!token.substr(0, pos2).compare("logFile"))
            {
                token.erase(0, pos2 + pathDelim.length());
                logPath = token;
            }
            else if (!token.substr(0, pos2).compare("portNo"))
            {
                token.erase(0, pos2 + pathDelim.length());
                portNo = stoi(token);
            }
        }
    }

    printf("%s\n", commandsPath.c_str());
    printf("%s\n", logPath.c_str());
    printf("%i\n", portNo);

    jint rest = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (rest != JNI_OK || jvmti == NULL) {

        printf("Unable to get access to JVMTI version 1.2");
        return JNI_ERR;
    }

    jvmtiCapabilities capa;
    jvmtiError error;
    memset(&capa, 0, sizeof(jvmtiCapabilities));
    capa.can_signal_thread = 1;
    capa.can_get_owned_monitor_info = 1;
    capa.can_generate_method_entry_events = 1;
    capa.can_tag_objects = 1;
    capa.can_get_current_thread_cpu_time = 1;
    capa.can_get_line_numbers = 1;
    capa.can_generate_vm_object_alloc_events = 1;
    capa.can_generate_monitor_events = 1;
    capa.can_generate_exception_events = 1;
    capa.can_get_source_file_name = 1;
    error = jvmti->AddCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Failed to set jvmtiCapabilities.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM init event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM death eventVerboseLogSubscriber.");

    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
    callbacks.VMInit = &VMInit;
    callbacks.VMDeath = &VMDeath;
    callbacks.VMObjectAlloc = &VMObjectAlloc;
    callbacks.MonitorContendedEntered = &MonitorContendedEntered;
    callbacks.MethodEntry = &MethodEntry;
    callbacks.Exception = &Exception;
    error = jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks.");

    return JNI_OK;
}
