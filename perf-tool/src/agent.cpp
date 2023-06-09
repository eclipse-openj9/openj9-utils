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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
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

jvmtiEnv *jvmti = NULL;
JNIEnv *jni_env = NULL;

/* Server arguments with defaults
 * These will be set by parseAgentOptions
 */
int portNo = ServerConstants::DEFAULT_PORT;
std::string commandsPath = "";
std::string logPath = "logs.json";
int verbose = NONE;
bool enableMethodEnterCapability = false;

void parseSingleOption(std::string token)
{
    const std::string pathDelim = ":";
    auto pos = token.find(pathDelim);
    if (pos != std::string::npos)
        {
        std::string optionName = token.substr(0, pos);
        std::string optionValue = token.substr(pos + pathDelim.length());
        if (!optionName.compare("commandFile"))
        {
            commandsPath = optionValue;
        }
        else if (!optionName.compare("logFile"))
        {
            logPath = optionValue;
        }
        else if (!optionName.compare("portNo"))
        {
            portNo = stoi(optionValue);
        }
        else if (!optionName.compare("verbose"))
        {
            verbose = static_cast<Verbose>(stoi(optionValue));
        }
        else if (!optionName.compare("methodEnterCapability"))
        {
            enableMethodEnterCapability = (optionValue.compare("on") == 0 ? true : false);
        }
        else
        {
            fprintf(stderr, "Warning: ignoring unknown option: %s\n", optionName.c_str());
        }
    }
    else
    {
        fprintf(stderr, "Warning: ignoring option because no value found for it: %s\n", token.c_str());
    }
}

void parseAgentOptions(const char *options)
{
    const std::string optionsDelim = ",";
    std::string oIn(options);
    int pos1 = 0;
    
    /* Possible options the user can supply here:
     * "commands" is followed by a path to the commands file
     * "log" is followed by the path to the location for the log file
     * "portNo" is followed by a number indicating the port to run the server on
     * "verbose" is used to indicate the degree of verbosity to show the additional information
     * "methodEnterCapability" is followed by "on" of "off" (default is "off")
     */
    while ((pos1 = oIn.find(optionsDelim)) != std::string::npos)
    {
        std::string token = oIn.substr(0, pos1);
        parseSingleOption(token);
        oIn.erase(0, pos1 + optionsDelim.length());
    }
    if (!oIn.empty())
    {
        parseSingleOption(oIn);
    }

    if (verbose != ERROR && verbose != WARN && verbose != INFO)
    {
        verbose = NONE;
    }
    
    if (verbose == INFO)
    {
        printf("verbosity: %i\n", verbose); 
        printf("path to command: %s\n", commandsPath.c_str());
        printf("logpath: %s\n", logPath.c_str());
        printf("port number: %i\n", portNo);
        printf("methodEnter/Exit capability enabled: %d\n", enableMethodEnterCapability);
    }
}

    
jvmtiError setCallbacks(jvmtiEnv *jvmti)
{
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
    callbacks.VMInit = &VMInit;
    callbacks.VMDeath = &VMDeath;
    callbacks.VMObjectAlloc = &VMObjectAlloc;
    callbacks.MonitorContendedEnter = &MonitorContendedEnter;
    callbacks.MonitorContendedEntered = &MonitorContendedEntered;
    callbacks.MethodEntry = &MethodEntry;
    callbacks.MethodExit = &MethodExit;
    callbacks.Exception = &Exception;
    jvmtiError error = jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks.");
    return error;
}


JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm, char *options, void *reserved)
{
    jint rc;
    /* If a previous attach operation loaded the library and created
     * the server, don't try to do it again
     */
    if (!server)
    {
        jvmtiError error;
        parseAgentOptions(options);
        if (verbose >= WARN)
            printf("On attach initiated with options: %s\n", options);
	
        rc = vm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_2);
        if (rc != JNI_OK)
        {
            if (verbose >= ERROR)
                fprintf(stderr, "Cannot get JVMTI env\n");
            return rc;
        }
        rc = vm->GetEnv((void **)&jni_env, JNI_VERSION_1_8);
        if (rc != JNI_OK)
        {
            if (verbose >= ERROR)	
                fprintf(stderr, "Cannot get JNI env\n");
            return rc;
        }

        ExtensionFunctions::getExtensionFunctions(jvmti);

        error = setCallbacks(jvmti);
        if (error != JVMTI_ERROR_NONE)
            return JNI_ERR; /* without callbacks I cannot do anything */

        error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to init VM death event.");


        /* Create the server and start the server thread */
        server = new Server(portNo, commandsPath, logPath);
        error = jvmti->RunAgentThread( createNewThread(jni_env), &startServer, NULL, JVMTI_THREAD_NORM_PRIORITY );
        if (error != JVMTI_ERROR_NONE)
        {
            if (verbose >= ERROR)	
                fprintf(stderr, "Error starting agent thread\n");
            delete server;
            server = NULL;
            return JNI_ERR;
        }
    }
    return JNI_OK;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    parseAgentOptions(options);
    if (verbose >= WARN)
        printf("Options passed in: %s\n", options);

    jint rest = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (rest != JNI_OK || jvmti == NULL) {
       
        if (verbose >= ERROR)    
        printf("Unable to get access to JVMTI version 1.2");
        return JNI_ERR;
    }

    ExtensionFunctions::getExtensionFunctions(jvmti);

    jvmtiError error = setCallbacks(jvmti);
    if (error != JVMTI_ERROR_NONE)
        return JNI_ERR;

    if (enableMethodEnterCapability)
        {
        jvmtiCapabilities capa;
        memset(&capa, 0, sizeof(jvmtiCapabilities));
        capa.can_generate_method_entry_events = 1; /* this one can only be added during the load phase */
        capa.can_generate_method_exit_events = 1;
        error = jvmti->AddCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to init MethodEnter/Exit capability.");
        }

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM init event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM death event.");

    return JNI_OK;
}
