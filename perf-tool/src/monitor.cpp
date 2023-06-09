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

#include <atomic>
#include <iostream>
#include <mutex>
#include <jvmti.h>
#include <ibmjvmti.h>
#include <map>
#include <unordered_map>
#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"

//how many times
using json = nlohmann::json;

std::atomic<int> monitorSampleCount{0};
std::atomic<int> monitorEnterSampleCount{0};
std::unordered_map<int, int> m_monitor;
thread_local long long int startTime;
thread_local jint objHash;
std::mutex monitorMapMutex;
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

    /* calculate timestamp when thread acquired monitor */
    auto currentClockTime = std::chrono::system_clock::now();
    long long int nano = std::chrono::duration_cast<std::chrono::nanoseconds>(currentClockTime.time_since_epoch()).count();
    /* Calculate lock latency */
    if (hash == objHash) {
        j["lockLatency"] = nano - startTime;
    }

    /*monitor for protecting hashtable*/
    int val = 1;
    {
        std::lock_guard<std::mutex> lg(monitorMapMutex);
        val = ++(m_monitor[hash]);
    }
    j["per_object_contended_count"] = val;

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
            return;
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
                return;
            }
            /* Now get the c string from the java jstring object */
            const char *str = env->GetStringUTFChars(strObj, NULL);
            if (str)
            {
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
                    fprintf(stderr, "%s:%d ERROR: Failed to get UTF-8 characters\n", __FILE__, __LINE__);
            }
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
    getThreadName(jvmtiEnv, thread, j);
    /* Get Java thread ID */
    getThreadID(env, thread, j);
    /* Get OS thread ID */
    getOSThreadID(jvmtiEnv, thread, j);
    /* get waiters info if required*/
    if (monitorConfig.getWaitersInfo())
    {
        jvmtiMonitorUsage monitor_usage;
        error = jvmtiEnv->GetObjectMonitorUsage(object, &monitor_usage);
        if (check_jvmti_error(jvmtiEnv, error, "Unable to get Monitor usage info."))
        {
            j["waitersCount"] = monitor_usage.waiter_count;
            auto waiters_list = json::array();
            for (int i = 0; i < monitor_usage.waiter_count; i++)
            {
                /* Get the waiter thread Details */
                json j_waiter;
                getThreadName(jvmtiEnv, monitor_usage.waiters[i], j_waiter);
                getThreadID(env, monitor_usage.waiters[i], j_waiter);
                getOSThreadID(jvmtiEnv, monitor_usage.waiters[i], j_waiter);
                waiters_list.push_back(j_waiter);
            }
            j["waiters"] = waiters_list;
            error = jvmtiEnv->Deallocate((unsigned char *)(monitor_usage.waiters));
            check_jvmti_error(jvmtiEnv, error, "Unable to deallocate waiters.\n");
            error =  jvmtiEnv->Deallocate((unsigned char *)(monitor_usage.notify_waiters));
            check_jvmti_error(jvmtiEnv, error, "Unable to deallocate notify waiters.\n");
        }
    }
    sendToServer(j, "monitorContendedEnteredEvent");

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
    err = jvmtiEnv->GetObjectHashCode(object, &objHash);
    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve object hashcode.")) 
    {
        char str[32];
        sprintf(str, "0x%x", objHash);
        j["monitorHash"] = str;
    }

    /* Timestamp of thread waiting to acquire monitor */
    auto currentClockTime = std::chrono::system_clock::now();
    startTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentClockTime.time_since_epoch()).count();

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
            return;
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
                return;
            }
            /* Now get the c string from the java jstring object */
            const char *str = env->GetStringUTFChars(strObj, NULL);
            if (str)
            {
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
                    fprintf(stderr, "%s:%d ERROR: Failed to get UTF-8 characters\n", __FILE__, __LINE__);
            }
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
            getThreadName(jvmtiEnv, monitor_usage.owner, jOwner);
            /* Get Java thread ID */
            getThreadID(env, monitor_usage.owner, jOwner);
            /* Get OS thread ID */
            getOSThreadID(jvmtiEnv, monitor_usage.owner, jOwner);
            j["OwnerThread"] = jOwner;
        }
    }
    json jCurrent;
    /* Get StackTrace */
    monitorConfig.getStackTrace(jvmtiEnv, thread, jCurrent, stackTraceDepth);
    /* Get the current thread name */
    getThreadName(jvmtiEnv, thread, jCurrent);
    /* Get Java thread ID */
    getThreadID(env, thread, jCurrent);
    /* Get OS thread ID */
    getOSThreadID(jvmtiEnv, thread, jCurrent);
    j["CurrentThread"] = jCurrent;
    sendToServer(j, "MonitorContendedEnterEvent");

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
