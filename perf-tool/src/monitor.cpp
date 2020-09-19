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

#include <atomic>
#include <iostream>
#include <jvmti.h>
#include <ibmjvmti.h>
#include <map>
#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"

//how many times
using json = nlohmann::json;
struct ClassCycleInfo
{
    int numFirstTier;
    // to be used for tiered spinning
    int numSecondTier;
    int numThirdTier;
};
std::atomic<bool> stackTraceEnabled{true};
std::atomic<int> monitorSampleRate{1};
std::atomic<int> monitorSampleCount{0};

void setMonitorStackTrace(bool val)
{
    /* Enables or disables the stack trace option */
    stackTraceEnabled = val;
}

void setMonitorSampleRate(int rate)
{
    if (rate > 0)
    {
        stackTraceEnabled = true;
        monitorSampleRate = rate;
    }
    else
    {
        stackTraceEnabled = false;
    }
}

JNIEXPORT void JNICALL MonitorContendedEntered(jvmtiEnv *jvmtiEnv, JNIEnv *env, jthread thread, jobject object){
    json j;
    jvmtiError error;
    static std::map<const char *, ClassCycleInfo> numContentions;
    jclass cls = env->GetObjectClass(object);
    /* First get the class object */
    jmethodID mid = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = env->CallObjectMethod(object, mid);
    /* Now get the class object's class descriptor */
    cls = env->GetObjectClass(clsObj);
    /* Find the getName() method on the class object */
    mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    /* Call the getName() to get a jstring object back */ 
    jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);
    /* Now get the c string from the java jstring object */
    const char *str = env->GetStringUTFChars(strObj, NULL);
    /* record calling class */
    j["Class"] = str;
    
    /* free str */


    numContentions[str].numFirstTier++;

    int num = numContentions[str].numFirstTier;
    j["numTypeContentions"] = num;
    /* Release the memory pinned char array */
    env->ReleaseStringUTFChars(strObj, str);
    env->DeleteLocalRef(cls);


    int numMonitors;
    /* Get number of methods and increment */
    numMonitors = atomic_fetch_add(&monitorSampleCount, 1);

    if (true)
    { /* only run if the backtrace is enabled */
    jvmtiError err;
        if (numMonitors % monitorSampleRate == 0)
        {
            jvmtiFrameInfo frames[5];
            jint count;
            err = jvmtiEnv->GetStackTrace(thread, 0, 5,
                                          frames, &count);
            if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Stack Trace.") && count >= 1)
            {
                char *methodName;
                err = jvmtiEnv->GetMethodName(frames[0].method,
                                              &methodName, NULL, NULL);
                if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Name.\n")) {
                    j["Method"] = methodName;
                    err = jvmtiEnv->Deallocate((unsigned char*)methodName);
                    check_jvmti_error(jvmtiEnv, err, "Unable to deallocate methodName.\n");
                }
                
            }
        }
        monitorSampleCount = atomic_fetch_add(&monitorSampleCount, 1);
    }
    sendToServer(j.dump());
}
