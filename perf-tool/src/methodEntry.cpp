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
#include <string.h>
#include "agentOptions.hpp"
#include "methodEntry.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.hpp"

#include <iostream>
#include <atomic>
#include <regex>

using json = nlohmann::json;

EventConfig methodEnterConfig;
EventConfig methodExitConfig;

std::atomic<int> mEntrySampleCount {0};
std::atomic<int> mExitSampleCount {0};


/* retrieves method name and line number, and declaring class name and signature
 *      for every nth method entry                                                 */
JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
                                   JNIEnv* env,
                                   jthread thread,
                                   jmethodID method) 
{
    /* Get number of methods and increment */
    int numMethods = atomic_fetch_add(&mEntrySampleCount, 1);

    if (numMethods % methodEnterConfig.getSampleRate() == 0) 
    {          
        json j;
        j["methodNum"] = numMethods;

        /* retrieve the filter, if any */
        auto filter = methodEnterConfig.getFilter();
        bool classFilter = std::get<0>(filter);
        bool methodFilter = std::get<1>(filter);
        bool signatureFilter = std::get<2>(filter);
        
        char *name_ptr;
        char *signature_ptr;
        jvmtiError err = jvmtiEnv->GetMethodName(method, &name_ptr, &signature_ptr, NULL);
        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Name.\n")) 
        {
            bool abort = false;
            std::regex &methodRegex    = std::get<4>(filter);
            std::regex &signatureRegex = std::get<5>(filter);
            if ((methodFilter    && !std::regex_search(name_ptr, methodRegex)) ||
                (signatureFilter && !std::regex_search(signature_ptr, signatureRegex)))
                abort = true;
            
            if (!abort)
            {
                j["methodName"] = name_ptr;
                j["methodSig"] = signature_ptr;
            }
            err = jvmtiEnv->Deallocate((unsigned char*)name_ptr);
            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate name_ptr.\n");
            err = jvmtiEnv->Deallocate((unsigned char*)signature_ptr);
            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate signature_ptr.\n");
            if (abort)
                return;
        }
        else
        {
            /* Without the method name, there is no point in continuing to get the class */
            return; 
        }

        jclass declaring_class;
        err = jvmtiEnv->GetMethodDeclaringClass(method, &declaring_class);
        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class.\n")) 
        {
            char *declaringClassName;
            err = jvmtiEnv->GetClassSignature(declaring_class, &declaringClassName, NULL);
            if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class Signature.\n")) 
            {
                std::regex &classRegex = std::get<3>(filter);
                bool abort = classFilter && !std::regex_search(declaringClassName, classRegex);
                if (!abort)
                    j["methodClass"] = declaringClassName;
                err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);
                check_jvmti_error(jvmtiEnv, err, "Unable to deallocate declaringClassName.\n");
                if (abort)
                    return;
            }
        }

        /* Get StackTrace */
        if (methodEnterConfig.getStackTraceDepth() > 1)
            methodEnterConfig.getStackTrace(jvmtiEnv, thread, j, methodEnterConfig.getStackTraceDepth());
/*
        The following code needs "can_get_line_numbers" capability.
        Due to the high frequency of MethodEnter events it's better to keep
        the overhead down by not getting information about the line numbers.
        This can be added in the future through an addional option.

        jint entry_count_ptr;
        jvmtiLineNumberEntry* table_ptr;
        err = jvmtiEnv->GetLineNumberTable(method, &entry_count_ptr, &table_ptr);
        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Line Number Table.\n")) 
        {
            j["methodLineNum"] = table_ptr->line_number;
            err = jvmtiEnv->Deallocate((unsigned char*)table_ptr);
            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate table_ptr.\n");
        }
*/
        sendToServer(j, "methodEntryEvent");
    }
}


JNIEXPORT void JNICALL MethodExit(jvmtiEnv *jvmtiEnv,
                                  JNIEnv* env,
                                  jthread thread,
                                  jmethodID method,
                                  jboolean was_popped_by_exception,
                                  jvalue return_value)
{
    /* Get number of methods and increment */
    int numMethods = atomic_fetch_add(&mExitSampleCount, 1);

    if (numMethods % methodExitConfig.getSampleRate() == 0) 
    {          
        json j;
        j["methodNum"] = numMethods;

        /* retrieve the filter, if any */
        auto filter = methodExitConfig.getFilter();
        bool classFilter = std::get<0>(filter);
        bool methodFilter = std::get<1>(filter);
        bool signatureFilter = std::get<2>(filter);
        
        char *name_ptr;
        char *signature_ptr;
        jvmtiError err = jvmtiEnv->GetMethodName(method, &name_ptr, &signature_ptr, NULL);
        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Name.\n")) 
        {
            bool abort = false;
            std::regex &methodRegex    = std::get<4>(filter);
            std::regex &signatureRegex = std::get<5>(filter);
            if ((methodFilter    && !std::regex_search(name_ptr, methodRegex)) ||
                (signatureFilter && !std::regex_search(signature_ptr, signatureRegex)))
                abort = true;

            if (!abort)
            {
                j["methodName"] = name_ptr;
                j["methodSig"] = signature_ptr;
            }
            err = jvmtiEnv->Deallocate((unsigned char*)name_ptr);
            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate name_ptr.\n");
            err = jvmtiEnv->Deallocate((unsigned char*)signature_ptr);
            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate signature_ptr.\n");
            if (abort)
                return;
        }
        else
        {
            /* Without the method name, there is no point in continuing to get the class */
            return; 
        }

        jclass declaring_class;
        err = jvmtiEnv->GetMethodDeclaringClass(method, &declaring_class);
        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class.\n")) 
        {
            char *declaringClassName;
            err = jvmtiEnv->GetClassSignature(declaring_class, &declaringClassName, NULL);
            if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class Signature.\n")) 
            {
                std::regex &classRegex = std::get<3>(filter);
                bool abort = classFilter && !std::regex_search(declaringClassName, classRegex);
                if (!abort)
                    j["methodClass"] = declaringClassName;
                err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);
                check_jvmti_error(jvmtiEnv, err, "Unable to deallocate declaringClassName.\n");
                if (abort)
                    return;
            }
        }

        /* Get StackTrace */
        if (methodExitConfig.getStackTraceDepth() > 1)
            methodExitConfig.getStackTrace(jvmtiEnv, thread, j, methodExitConfig.getStackTraceDepth());

        sendToServer(j, "methodExitEvent");
    }
}
