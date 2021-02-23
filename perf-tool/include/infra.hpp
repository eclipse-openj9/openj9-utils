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

#ifndef INFRA_H_
#define INFRA_H_

#include <algorithm>
#include <mutex>
#include <string>
#include <jvmti.h>

class Server;
extern Server *server;
extern JNIEnv *jni_env;

void check_jvmti_error_throw(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

/* returns False if error detected. */
bool check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread);
JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env);
jthread createNewThread(JNIEnv* jni_env);
void JNICALL startServer(jvmtiEnv * jvmti, JNIEnv* jni, void *p);
void sendToServer(std::string message);

extern int portNo;
extern std::string commandsPath;
extern std::string logPath;

class EventConfig
{
public:
    struct CallbackIDs 
    {
        jclass cachedCallbackClass = NULL;
        jmethodID cachedCallbackMethodId = NULL;
    };
private:
    int sampleRate = 1;
    int stackTraceDepth = 0;
    std::string callbackClass;
    std::string callbackMethod;
    std::string callbackSignature;
    CallbackIDs callbackIDs;
    std::mutex configMutex;
    #define MAX_TRACE_DEPTH 128
public:
    static constexpr int getMaxTraceDepth() { return MAX_TRACE_DEPTH; } 
    int getSampleRate(void) const { return sampleRate; }
    void setSampleRate(int rate) { sampleRate = (rate > 0 ? rate : 1); }
    int getStackTraceDepth() const { return stackTraceDepth; }
    void setStackTraceDepth(int depth) { stackTraceDepth = std::min(depth, MAX_TRACE_DEPTH); }
    const std::string& getCallbackClass() const { return callbackClass; }
    const std::string& getCallbackMethod() const { return callbackMethod; }
    const std::string &getCallbackSignature() const { return callbackSignature; }
    void setCallbacks(const std::string& cls, const std::string& method, const std::string& sig);
    jclass getCachedCallbackClass() const { return callbackIDs.cachedCallbackClass; }
    jmethodID getCachedCallbackMethodId() const { return callbackIDs.cachedCallbackMethodId; }
    CallbackIDs getCallBackIDs(JNIEnv *env);
};


#endif /* INFRA_H_ */
