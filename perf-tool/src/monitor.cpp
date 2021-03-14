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
#include <mutex>
#include <jvmti.h>
#include <ibmjvmti.h>
#include <map>
#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"

//how many times
using json = nlohmann::json;

std::atomic<int> monitorSampleCount{0};
std::atomic<int> monitorEnterSampleCount{0};
EventConfig monitorConfig;

JNIEXPORT void JNICALL MonitorContendedEntered(jvmtiEnv *jvmtiEnv, JNIEnv *env, jthread thread, jobject object){
    json j;
    jvmtiError error;

    /* Get number of events and increment */
    int numSamples = atomic_fetch_add(&monitorSampleCount, 1);
    if (numSamples % monitorConfig.getSampleRate() != 0)
        return;

    jvmtiError err;
    jint hash;
    err = jvmtiEnv->GetObjectHashCode(object, &hash);
    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve object hashcode."))
    {
        char str[32];
        sprintf(str, "0x%x", hash);
        j["monitorHash"] = str;
    }

    static std::map<std::string, int> numContentions;
    jclass cls = env->GetObjectClass(object);
    /* First get the class object */
    jmethodID mid = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");
    if (mid)
    {
        jobject clsObj = env->CallObjectMethod(object, mid);
        if (env->ExceptionCheck() == JNI_TRUE)
        {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        /* Now get the class object's class descriptor */
        cls = env->GetObjectClass(clsObj);
        /* Find the getName() method on the class object */
        mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
        if (mid)
        {
            /* Call the getName() to get a jstring object back */
            jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);
            if (env->ExceptionCheck() == JNI_TRUE)
            {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            /* Now get the c string from the java jstring object */
            const char *str = env->GetStringUTFChars(strObj, NULL);
            /* record calling class */
            j["Class"] = str;
            static std::mutex contentionMutex;
            int samplesPerObjectType = 0;
            {
            std::lock_guard<std::mutex> lg(contentionMutex);
            samplesPerObjectType = ++(numContentions[std::string(str)]);
            }
            j["numTypeContentions"] = samplesPerObjectType;
            /* Release the memory pinned char array */
            env->ReleaseStringUTFChars(strObj, str);
            env->DeleteLocalRef(cls);
        }
        else
        {
            if (verbose >= ERROR)
                fprintf (stderr, "Error calling GetMethodID for java/lang/String.getName()\n");
        }
    }
    else
    {
        if (verbose >= ERROR)
            fprintf (stderr, "Error calling GetMethodID for java/lang/Class.getClass()\n");
    }
    /* Get StackTrace */
    monitorConfig.getStackTrace(jvmtiEnv, thread, j, monitorConfig.getStackTraceDepth());
    /* Get the thread name */
    jvmtiThreadInfo threadInfo;
    error = jvmtiEnv->GetThreadInfo(thread, &threadInfo);
    if (check_jvmti_error(jvmtiEnv, error, "Unable to retrieve thread info."))
    {
        j["threadName"] = threadInfo.name;
        error = jvmtiEnv->Deallocate((unsigned char*)(threadInfo.name));
        check_jvmti_error(jvmtiEnv, error, "Unable to deallocate thread name.\n");
        // Local JNI refs to threadInfo.thread_group and threadInfo.context_class_loader will be freed upon return
    }
    /* Get Java thread ID */
    jclass threadClass = env->FindClass("java/lang/Thread");
    if (threadClass)
    {
        jmethodID getIdMethod = env->GetMethodID(threadClass, "getId", "()J");
        if (getIdMethod)
        {
            jlong tid = env->CallLongMethod(thread, getIdMethod);
            j["threadID"] = tid; 
        }
        else
        {
            if (verbose >= ERROR)
                fprintf (stderr, "Error calling GetMethodID for java/lang/Thread.getId()J\n");
        }
    }
    else
    {
        printf ("Error calling FindClass for java/lang/Thread\n");
    }

    /* Get OS thread ID 
     * We need to use OpenJ9 JVMTI extension functions
     */
    jint extensionFunctionCount = 0;
    jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
    jvmtiEnv->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);
    for (int i=0; i < extensionFunctionCount; i++) /* search for desired function */
    {
        jvmtiExtensionFunction function = extensionFunctions->func;
        if (strcmp(extensionFunctions->id, COM_IBM_GET_OS_THREAD_ID) == 0)
        {
            jlong threadID = 0;
            /* jvmtiError GetOSThreadID(jvmtiEnv* jvmti_env, jthread thread, jlong * threadid_ptr); */
            error = function(jvmtiEnv, thread, &threadID);
            if (check_jvmti_error(jvmtiEnv, error, "Unable to retrieve thread ID."))
            {
                j["threadNativeID"] = threadID;
            }
            break;
        }
        extensionFunctions++; /* move on to the next extension function */
    }
    
    sendToServer(j.dump());

    /* Also call the callback */
    
    EventConfig::CallbackIDs callbackIDs = monitorConfig.getCallBackIDs(env);
    if (callbackIDs.cachedCallbackClass && callbackIDs.cachedCallbackMethodId)
    {
        jstring jsonEntry = env->NewStringUTF(j.dump().c_str());
        if (jsonEntry)
        { 
            env->CallStaticVoidMethod(callbackIDs.cachedCallbackClass, callbackIDs.cachedCallbackMethodId, jsonEntry);
            /* If the callback generates an exception that is not caught
             * suppress that exception from being propagated further
             * because it can be confusing 
             */
            if (env->ExceptionCheck() == JNI_TRUE)
            {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
        }
    }
}
JNIEXPORT void JNICALL MonitorContendedEnter(jvmtiEnv *jvmtiEnv, JNIEnv *env, jthread thread, jobject object){
    json j;
    jvmtiError error;
    
    /* Get number of events and increment */
    int numSamples = atomic_fetch_add(&monitorEnterSampleCount, 1);
    if (numSamples % monitorConfig.getSampleRate() != 0)
        return;

    jvmtiError err;
    jint hash;
    err = jvmtiEnv->GetObjectHashCode(object, &hash);
    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve object hashcode.")) 
    {
        char str[32];
        sprintf(str, "0x%x", hash);
        j["monitorHash"] = str;
    }

    static std::map<std::string, int> numContentions;
    jclass cls = env->GetObjectClass(object);
    /* First get the class object */
    jmethodID mid = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");
    if (mid)
    {
        jobject clsObj = env->CallObjectMethod(object, mid);
        if (env->ExceptionCheck() == JNI_TRUE)
        {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        /* Now get the class object's class descriptor */
        cls = env->GetObjectClass(clsObj);
        /* Find the getName() method on the class object */
        mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
        if (mid) 
        {
            /* Call the getName() to get a jstring object back */
            jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);
            if (env->ExceptionCheck() == JNI_TRUE)
            {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            /* Now get the c string from the java jstring object */
            const char *str = env->GetStringUTFChars(strObj, NULL);
            /* record calling class */
            j["class"] = str;
            static std::mutex contentionMutex;
            int samplesPerObjectType = 0;
            {
            std::lock_guard<std::mutex> lg(contentionMutex);
            samplesPerObjectType = ++(numContentions[std::string(str)]);
            }
            j["numTypeContentions"] = samplesPerObjectType;
            /* Release the memory pinned char array */
            env->ReleaseStringUTFChars(strObj, str);
            env->DeleteLocalRef(cls);
        }
        else
        {
            if (verbose >= ERROR)
                fprintf (stderr, "Error calling GetMethodID for java/lang/String.getName()\n");
        }
    }
    else
    {
        if (verbose >= ERROR)
            fprintf (stderr, "Error calling GetMethodID for java/lang/Class.getClass()\n");
    }
    jvmtiMonitorUsage monitor_usage;
    jint stackTraceDepth = monitorConfig.getStackTraceDepth();
    error = jvmtiEnv->GetObjectMonitorUsage(object, &monitor_usage);
    if (check_jvmti_error(jvmtiEnv, error, "Unable to get Monitor usage info."))
    {
        error = jvmtiEnv->Deallocate((unsigned char *)(monitor_usage.waiters));
        check_jvmti_error(jvmtiEnv, error, "Unable to deallocate waiters.\n");
        error =  jvmtiEnv->Deallocate((unsigned char *)(monitor_usage.notify_waiters));
        check_jvmti_error(jvmtiEnv, error, "Unable to deallocate notify waiters.\n");
        if (monitor_usage.owner != NULL)
        {
            json jOwner;
            /* Get StackTrace */
            monitorConfig.getStackTrace(jvmtiEnv, monitor_usage.owner, jOwner, stackTraceDepth);
            /* Get the thread name */
            jvmtiThreadInfo threadInfo;
            error = jvmtiEnv->GetThreadInfo(monitor_usage.owner, &threadInfo);
            if (check_jvmti_error(jvmtiEnv, error, "Unable to retrieve thread info."))
            {
                jOwner["threadName"] = threadInfo.name;
                error = jvmtiEnv->Deallocate((unsigned char*)(threadInfo.name));
                check_jvmti_error(jvmtiEnv, error, "Unable to deallocate thread name.\n");
                // Local JNI refs to threadInfo.thread_group and threadInfo.context_class_loader will be freed upon return
            }
            /* Get Java thread ID */
            jclass threadClass = env->FindClass("java/lang/Thread");
            if (threadClass)
            {
                jmethodID getIdMethod = env->GetMethodID(threadClass, "getId", "()J");
                if (getIdMethod)
                {
                    jlong tid = env->CallLongMethod(monitor_usage.owner, getIdMethod);
                    jOwner["threadID"] = tid;
                }
                else
                {
                    printf ("Error calling GetMethodID for java/lang/Thread.getId()J\n");
                }
            }
            else
            {
                printf ("Error calling FindClass for java/lang/Thread\n");
            }
            /* Get OS thread ID
             * We need to use OpenJ9 JVMTI extension functions
             */
            jint extensionFunctionCount = 0;
            jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
            jvmtiEnv->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);
            for (int i=0; i < extensionFunctionCount; i++) /* search for desired function */
            {
                jvmtiExtensionFunction function = extensionFunctions->func;
                if (strcmp(extensionFunctions->id, COM_IBM_GET_OS_THREAD_ID) == 0)
                {
                    jlong threadID = 0;
                    /* jvmtiError GetOSThreadID(jvmtiEnv* jvmti_env, jthread thread, jlong * threadid_ptr); */
                    error = function(jvmtiEnv, monitor_usage.owner, &threadID);
                    if (check_jvmti_error(jvmtiEnv, error, "Unable to retrieve thread ID."))
                    {
                        jOwner["threadNativeID"] = threadID;
                    }
                    break;
                }
                extensionFunctions++; /* move on to the next extension function */    
            }
            j["OwnerThread"] = jOwner;
        }
    }
    json jCurrent;
    /* Get StackTrace */
    monitorConfig.getStackTrace(jvmtiEnv, thread, jCurrent, stackTraceDepth);
    /* Get the current thread name */
    jvmtiThreadInfo threadInfo;
    error = jvmtiEnv->GetThreadInfo(thread, &threadInfo);
    if (check_jvmti_error(jvmtiEnv, error, "Unable to retrieve thread info."))
    {
        jCurrent["threadName"] = threadInfo.name;
        error = jvmtiEnv->Deallocate((unsigned char*)(threadInfo.name));
        check_jvmti_error(jvmtiEnv, error, "Unable to deallocate thread name.\n");
        // Local JNI refs to threadInfo.thread_group and threadInfo.context_class_loader will be freed upon return
    }
    /* Get Java thread ID */
    jclass threadClass = env->FindClass("java/lang/Thread");
    if (threadClass)
    {
        jmethodID getIdMethod = env->GetMethodID(threadClass, "getId", "()J");
        if (getIdMethod)
        {
            jlong tid = env->CallLongMethod(thread, getIdMethod);
            jCurrent["threadID"] = tid;
        }
        else
        {
            printf ("Error calling GetMethodID for java/lang/Thread.getId()J\n");
        }
    }
    else
    {
        printf ("Error calling FindClass for java/lang/Thread\n");
    }
    /* Get OS thread ID
     * We need to use OpenJ9 JVMTI extension functions
     */
    jint extensionFunctionCount = 0;
    jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
    jvmtiEnv->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);
    for (int i=0; i < extensionFunctionCount; i++) /* search for desired function */
    {
        jvmtiExtensionFunction function = extensionFunctions->func;
        if (strcmp(extensionFunctions->id, COM_IBM_GET_OS_THREAD_ID) == 0)
        {
            jlong threadID = 0;
            /* jvmtiError GetOSThreadID(jvmtiEnv* jvmti_env, jthread thread, jlong * threadid_ptr); */
            error = function(jvmtiEnv, thread, &threadID);
            if (check_jvmti_error(jvmtiEnv, error, "Unable to retrieve thread ID."))
            {
                jCurrent["threadNativeID"] = threadID;
            }
            break;
        }
        extensionFunctions++; /* move on to the next extension function */
    }
    j["CurrentThread"] = jCurrent;
    sendToServer(j.dump());

    /* Also call the callback */

    EventConfig::CallbackIDs callbackIDs = monitorConfig.getCallBackIDs(env);
    if (callbackIDs.cachedCallbackClass && callbackIDs.cachedCallbackMethodId)
    {
        jstring jsonEntry = env->NewStringUTF(j.dump().c_str());
        if (jsonEntry)
        {
            env->CallStaticVoidMethod(callbackIDs.cachedCallbackClass, callbackIDs.cachedCallbackMethodId, jsonEntry);
            /* If the callback generates an exception that is not caught
             * suppress that exception from being propagated further
             * because it can be confusing
             */
            if (env->ExceptionCheck() == JNI_TRUE)
            {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
        }
    }
}
