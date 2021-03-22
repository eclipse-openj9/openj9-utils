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

// TODO: object count
#include <jvmti.h>
#include <string.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "json.hpp"
#include "server.hpp"
#include "infra.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <atomic>

using json = nlohmann::json;
using namespace std::chrono;

std::atomic<bool> objAllocBackTraceEnabled {true};
std::atomic<int> objAllocSampleCount {0};
std::atomic<int> objAllocSampleRate {1};

/* Enables or disables the back trace option if sampleRate == 0 */
void setObjAllocBackTrace(bool val){
    objAllocBackTraceEnabled = val;
    return;
}

/* to turn off backTrace, set sampleRate to 0 */
void setObjAllocSampleRate(int rate) {
    if (rate > 0) {
        objAllocBackTraceEnabled = true;
        objAllocSampleRate = rate;
    } else {
        objAllocBackTraceEnabled = false;
    }
    return;
}

/*** retrieves object type name, size (in bytes), allocation rate (bytes/microsec),
 *      and backtrace for every nth sample (if enabled)                             ***/
JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv,
                        JNIEnv* env,
                        jthread thread,
                        jobject object,
                        jclass object_klass,
                        jlong size) {
    jvmtiError err;

    json jObj;
    char *classType;
    auto start = steady_clock::now();


    int numObjects;

    /* Get number of objects and increment */
    numObjects = atomic_fetch_add(&objAllocSampleCount, 1);
    jObj["objNum"] = numObjects;

    /*** get information about object ***/
    err = jvmtiEnv->GetClassSignature(object_klass, &classType, NULL);
    if (classType != NULL && check_jvmti_error(jvmtiEnv, err, "Unable to retrive Object Class.\n")) {
        jObj["objType"] = classType;
        jObj["size"] = (jint)size;    
        err = jvmtiEnv->Deallocate((unsigned char*)classType);
        check_jvmti_error(jvmtiEnv, err, "Unable to deallocate classType.\n");
    }

    /*** get information about backtrace at object allocation sites if enabled***/
    /*** retrieves method names and line numbers, and declaring class name and signature ***/
    if (objAllocBackTraceEnabled) {
        if (numObjects % objAllocSampleRate == 0){
            int numFrames = 10;
            jvmtiFrameInfo frames[numFrames];
            jint count;
            int i;
            // int sze = 0;
            auto jMethods = json::array();
            err = jvmtiEnv->GetStackTrace(NULL, 0, numFrames, frames, &count);
            if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Stack Trace.\n") && count >= 1) {
                char *methodName;
                char *methodSignature;
                char *declaringClassName;
                jclass declaring_class;
                jint entry_count_ptr;
                jvmtiLineNumberEntry* table_ptr;
                json jMethod;
                for (i = 0; i < count; i++) {
                    jMethod["methodNum"] = i;
                    err = jvmtiEnv->GetMethodName(frames[i].method, &methodName, &methodSignature, NULL);
                    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Name.\n")) {
                        jMethod["methodName"] = methodName;
                        jMethod["methodSignature"] = methodSignature;
                        err = jvmtiEnv->Deallocate((unsigned char*)methodSignature);
                        check_jvmti_error(jvmtiEnv, err, "Unable to deallocate methodSignature.\n");
                        err = jvmtiEnv->Deallocate((unsigned char*)methodName);
                        check_jvmti_error(jvmtiEnv, err, "Unable to deallocate methodName.\n");
                    }
                    err = jvmtiEnv->GetMethodDeclaringClass(frames[i].method, &declaring_class);
                    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class.\n")) {
                        err = jvmtiEnv->GetClassSignature(declaring_class, &declaringClassName, NULL);
                        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class Signature.\n")) {
                            jMethod["methodClass"] = declaringClassName;
                            err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);
                            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate declaringClassName.\n");
                        } 
                    }
                    err = jvmtiEnv->GetLineNumberTable(frames[i].method, &entry_count_ptr, &table_ptr);
                    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Line Number Table.\n")) {
                        jMethod["methodLineNum"] = table_ptr[i].line_number;
                        err = jvmtiEnv->Deallocate((unsigned char*)table_ptr);
                        check_jvmti_error(jvmtiEnv, err, "Unable to deallocate table_ptr.\n");
                    }
                    
                    jMethods.push_back(jMethod);
                }
            } 
                
            jObj["objBackTrace"] = jMethods;
        }
        objAllocSampleCount = atomic_fetch_add(&objAllocSampleCount, 1);
    }

    /*** calculate time taken in microseconds and calculate rate ***/
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    float rate = (float)size/duration;
    jObj["objAllocRate"] = rate;

    json j;
    j["object"] = jObj; 
    sendToServer(j, "objectAllocationEvent");

}
