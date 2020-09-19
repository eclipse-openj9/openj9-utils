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
#include <thread>
#include <string.h>

#include "infra.hpp"
#include "server.hpp"

Server *server = NULL;

void check_jvmti_error_throw(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);
        printf("ERROR: JVMTI: [%d] %s - %s\n", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        throw "Oops!";
    }
}

bool check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str;
        errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);
        printf("ERROR: JVMTI: [%d] %s - %s\n", errnum, (errnum_str == NULL ? "Unknown": errnum_str), (str == NULL ? "" : str));
        return false;
    }
    return true;
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
    int* portPointer = portNo ? &portNo : NULL;

    server = new Server(portNo, commandsPath, logPath);

    error = jvmtiEnv -> RunAgentThread( createNewThread(jni_env),&startServer, portPointer, JVMTI_THREAD_NORM_PRIORITY );
    check_jvmti_error_throw(jvmtiEnv, error, "Error starting agent thread.");
    printf("VM starting up.\n");
}

JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env) {
    server->shutDownServer();
    delete server;
    printf("VM shutting down.\n");
}
