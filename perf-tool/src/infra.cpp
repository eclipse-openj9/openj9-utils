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
#include <thread>
#include <string.h>

#include "infra.hpp"
#include "server.hpp"
#include "json.hpp"

using json = nlohmann::json;
Server *server = NULL;

void check_jvmti_error_throw(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);
        if (verbose >= ERROR)
            fprintf(stderr, "ERROR: JVMTI: [%d] %s - %s\n", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        throw "Oops!";
    }
}

bool check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str;
        errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);
        if (verbose >= ERROR)
            fprintf(stderr, "ERROR: JVMTI: [%d] %s - %s\n", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        return false;
    }
    return true;
}

void EventConfig::getStackTrace(jvmtiEnv *jvmtiEnv, jthread thread, json& j, jint stackTraceDepth) {
    if (stackTraceDepth > 0)
    {
        jvmtiError err;
        jvmtiFrameInfo frames[EventConfig::getMaxTraceDepth()];
        jint count;
        err = jvmtiEnv->GetStackTrace(thread, 0, stackTraceDepth, frames, &count);
        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Stack Trace.") && count >= 1)
        {
            auto jMethods = json::array();
            bool error = false;
            for (int i=0; i < count && !error; i++)
            {
                char *methodName;
                char *signature;
                err = jvmtiEnv->GetMethodName(frames[i].method, &methodName, &signature, NULL);
                if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Name.\n"))
                {
                    jclass declaring_class;
                    err = jvmtiEnv->GetMethodDeclaringClass(frames[i].method, &declaring_class);
                    if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class.\n"))
                    {
                        char *declaringClassName;
                        err = jvmtiEnv->GetClassSignature(declaring_class, &declaringClassName, NULL);
                        if (check_jvmti_error(jvmtiEnv, err, "Unable to retrieve Method Declaring Class Signature.\n"))
                        {
                            json jMethod;
                            jMethod["class"] = declaringClassName;
                            jMethod["method"] = methodName;
                            jMethod["signature"] = signature;
                            jMethods.push_back(jMethod);
                            err = jvmtiEnv->Deallocate((unsigned char*)declaringClassName);
                            check_jvmti_error(jvmtiEnv, err, "Unable to deallocate class name.\n");
                        }
                        else
                        {
                            error = true;
                        }
                    }
                    else
                    {
                        error = true;
                    }
                    err = jvmtiEnv->Deallocate((unsigned char*)methodName);
                    check_jvmti_error(jvmtiEnv, err, "Unable to deallocate methodName.\n");
                    err = jvmtiEnv->Deallocate((unsigned char*)signature);
                    check_jvmti_error(jvmtiEnv, err, "Unable to deallocate method signature.\n");
                }
                else
                {
                    error = true;
                }
            }/* end for loop */
            j["stackTrace"] = jMethods;
        }
    }
    return;
}

void EventConfig::setCallbacks(const std::string& cls, const std::string& method, const std::string& sig)
{
    std::lock_guard<std::mutex> lg(configMutex);
    callbackClass.assign(cls);
    callbackMethod.assign(method);
    callbackSignature.assign(sig);
    /* Also reset the cached IDs; they need to be re-discovered */
    if (callbackIDs.cachedCallbackClass)
        jni_env->DeleteGlobalRef(callbackIDs.cachedCallbackClass);
    callbackIDs.cachedCallbackClass = NULL;
    callbackIDs.cachedCallbackMethodId = NULL;
}

EventConfig::CallbackIDs EventConfig::getCallBackIDs(JNIEnv *env)
{
    std::lock_guard<std::mutex> lg(configMutex);
    if (!getCachedCallbackClass() || !getCachedCallbackMethodId())
    {
        /* Need to compute them */
        if (!getCallbackClass().empty() && !getCallbackMethod().empty() && !getCallbackSignature().empty())
        {
            jclass callbackCls = env->FindClass(getCallbackClass().c_str());
            if (callbackCls)
            {
                jmethodID callbackMethodID = env->GetStaticMethodID(callbackCls, getCallbackMethod().c_str(), getCallbackSignature().c_str());
                if (callbackMethodID)
                {
                    jclass cls = (jclass)env->NewGlobalRef(callbackCls); /* If we cache it, we need to pin the class to avoid unloading */
                    callbackIDs.cachedCallbackClass = cls;
                    callbackIDs.cachedCallbackMethodId = callbackMethodID;
                }
                else
                {
                    if (env->ExceptionCheck() == JNI_TRUE)
                        env->ExceptionClear();
                    if (verbose >= ERROR)
                        fprintf(stderr, "Cannot find callback method %s%s\n", getCallbackMethod().c_str(), getCallbackSignature().c_str());
                    /* Delete the name of the method, so that we don't search over and over */
                    callbackMethod.clear();
                }
            }
            else
            {
                if (env->ExceptionCheck() == JNI_TRUE)
                    env->ExceptionClear();
                if (verbose >= ERROR)
                    fprintf(stderr, "Cannot find callback class %s\n", getCallbackClass().c_str());
                /* Delete the name of the class, so that we don't search over and over */
                callbackClass.clear();
            }
        }
    }
    return callbackIDs;
}


jvmtiExtensionFunction ExtensionFunctions::_jlmSet = NULL;
jvmtiExtensionFunction ExtensionFunctions::_jlmDump = NULL;
jvmtiExtensionFunction ExtensionFunctions::_jlmDumpStats = NULL;
jvmtiExtensionFunction ExtensionFunctions::_osThreadID = NULL;
jvmtiExtensionFunction ExtensionFunctions::_verboseGCSubcriber = NULL;
jvmtiExtensionFunction ExtensionFunctions::_verboseGCUnsubcriber = NULL;

void ExtensionFunctions::getExtensionFunctions(jvmtiEnv *jvmtiEnv)
{
    jint extensionFunctionCount = 0;
    jvmtiExtensionFunctionInfo *extensionFunctions = NULL;
    jvmtiError result = jvmtiEnv->GetExtensionFunctions(&extensionFunctionCount, &extensionFunctions);
    if (check_jvmti_error(jvmtiEnv, result, "Unable to retrieve extension functions"))
    {
        for (int i=0; i < extensionFunctionCount; i++) /* search for desired function */
        {
            if (strcmp(extensionFunctions[i].id, COM_IBM_SET_VM_JLM) == 0)
            {
                _jlmSet = extensionFunctions[i].func;
            }
            else if (strcmp(extensionFunctions[i].id, COM_IBM_SET_VM_JLM_DUMP) == 0)
            {
                _jlmDump = extensionFunctions[i].func;
            }
            else if (strcmp(extensionFunctions[i].id, COM_IBM_JLM_DUMP_STATS) == 0)
            {
                _jlmDumpStats = extensionFunctions[i].func;
            }
            else if (strcmp(extensionFunctions[i].id, COM_IBM_GET_OS_THREAD_ID) == 0)
            {
                _osThreadID = extensionFunctions[i].func;
            }
            else if (strcmp(extensionFunctions[i].id, COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER) == 0)
            {
                _verboseGCSubcriber = extensionFunctions[i].func;
            }
            else if (strcmp(extensionFunctions[i].id, COM_IBM_DEREGISTER_VERBOSEGC_SUBSCRIBER) == 0)
            {
                _verboseGCUnsubcriber = extensionFunctions[i].func;
            }
        }
        result = jvmtiEnv->Deallocate((unsigned char*)extensionFunctions);
        check_jvmti_error(jvmtiEnv, result, "Unable to deallocate extension functions");
        if (verbose >= Verbose::WARN)
        {
            if (!_jlmSet)
                fprintf(stderr, "Could not set COM_IBM_SET_VM_JLM extended function\n");
            if (!_jlmDump)
                fprintf(stderr, "Could not set COM_IBM_SET_VM_JLM_DUMP extended function\n");
            if (!_jlmDumpStats)
                fprintf(stderr, "Could not set COM_IBM_JLM_DUMP_STATS extended function\n");
            if (!_osThreadID)
                fprintf(stderr, "Could not set COM_IBM_GET_OS_THREAD_ID extended function\n");
            if (!_verboseGCSubcriber)
                fprintf(stderr, "Could not set COM_IBM_REGISTER_VERBOSEGC_SUBSCRIBER extended function\n");
            if (!_verboseGCUnsubcriber)
                fprintf(stderr, "Could not set COM_IBM_DEREGISTER_VERBOSEGC_SUBSCRIBER extended function\n");
        }
    }

}

jthread createNewThread(JNIEnv* jni_env){
    /* allocates a new java thread object using JNI */
    jclass threadClass;
    jmethodID id;
    jthread newThread;

    threadClass = jni_env->FindClass("java/lang/Thread");
    id = jni_env -> GetMethodID(threadClass, "<init>", "()V");
    newThread = jni_env -> NewObject(threadClass, id);
    return newThread;
}

void JNICALL startServer(jvmtiEnv * jvmti, JNIEnv* jni, void *p)
{
    server->handleServer();
}

void sendToServer(const std::string message)
{
    server->handleMessagingClients(message);
}

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread) {
    jvmtiError error;
    server = new Server(portNo, commandsPath, logPath);

    error = jvmtiEnv -> RunAgentThread( createNewThread(jni_env), &startServer, NULL, JVMTI_THREAD_NORM_PRIORITY );
    check_jvmti_error_throw(jvmtiEnv, error, "Error starting agent thread.");
    if (verbose >= WARN)
        printf("VM starting up.\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    if (server)
    {
        server->shutDownServer();
        delete server;
        server = NULL;
    }
    if (verbose >= WARN)
        printf("VM shutting down.\n");
}
