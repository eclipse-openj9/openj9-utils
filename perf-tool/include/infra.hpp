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
#include <regex>
#include <string>
#include <jvmti.h>
#include "json.hpp"

using json = nlohmann::json;

class Server;
extern Server *server;
extern JNIEnv *jni_env;

void check_jvmti_error_throw(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

/* returns False if error detected. */
bool check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

void getThreadName(jvmtiEnv *jvmtiEnv, jthread thread, json& j);
void getThreadID(JNIEnv *env, jthread thread, json& j);
void getOSThreadID(jvmtiEnv *jvmtiEnv, jthread thread, json& j);

JNIEXPORT void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env, jthread thread);
JNIEXPORT void JNICALL VMDeath(jvmtiEnv *jvmtiEnv, JNIEnv* jni_env);
jthread createNewThread(JNIEnv* jni_env);
void JNICALL startServer(jvmtiEnv * jvmti, JNIEnv* jni, void *p);
void sendToServer(const json& message, std::string event);

extern int portNo;
extern std::string commandsPath;
extern std::string logPath;

enum Verbose { NONE, ERROR, WARN, INFO};

extern int verbose;

struct ExtensionFunctions
{
    static jvmtiExtensionFunction _jlmSet;
    static jvmtiExtensionFunction _jlmDump;
    static jvmtiExtensionFunction _jlmDumpStats;
    static jvmtiExtensionFunction _osThreadID;
    static jvmtiExtensionFunction _verboseGCSubcriber;
    static jvmtiExtensionFunction _verboseGCUnsubcriber;
    static void getExtensionFunctions(jvmtiEnv *jvmtiEnv);
};

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
    bool waitersInfo = false;
    bool methodFilter = false;
    bool signatureFilter = false;
    bool classFilter = false;
    std::regex methodRegex;
    std::regex signatureRegex;
    std::regex classRegex; 
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
    bool getWaitersInfo() const { return waitersInfo; }
    void setWaitersInfo(bool waiters) { waitersInfo = waiters; }
    int getStackTraceDepth() const { return stackTraceDepth; }
    void setStackTraceDepth(int depth) { stackTraceDepth = std::min(depth, MAX_TRACE_DEPTH); }
    const std::string& getCallbackClass() const { return callbackClass; }
    const std::string& getCallbackMethod() const { return callbackMethod; }
    const std::string &getCallbackSignature() const { return callbackSignature; }
    void setCallbacks(const std::string& cls, const std::string& method, const std::string& sig);
    void setFilter(const std::string& cls, const std::string& method, const std::string& sig);
    std::tuple<bool, bool, bool, std::regex, std::regex, std::regex> getFilter();
    void getStackTrace(jvmtiEnv *jvmtiEnv, jthread thread, json& j, jint StackTraceDepth);
    jclass getCachedCallbackClass() const { return callbackIDs.cachedCallbackClass; }
    jmethodID getCachedCallbackMethodId() const { return callbackIDs.cachedCallbackMethodId; }
    CallbackIDs getCallBackIDs(JNIEnv *env);
};

/* Class used to change endianess: from host order to network order (big endian) */
class Host2Network
{
public:
    Host2Network()
    {
        int32_t val = 1;
        _isLittleEndian = *reinterpret_cast<uint8_t*>(&val) == 1 ? true : false;
    }
    uint32_t convert(uint32_t val)
    {
        if (_isLittleEndian)
        {
            return ((val << (8*3)) & 0xFF000000) |
                   ((val << (8*1)) & 0x00FF0000) |
                   ((val >> (8*1)) & 0x0000FF00) |
                   ((val >> (8*3)) & 0x000000FF);
        }
        else
        {
            return val;
        }
    }
    uint64_t convert(uint64_t val)
    {
        if (_isLittleEndian)
        {
            return ((val << (8*7)))  |
                   ((val << (8*5)) & 0x00FF000000000000ull) |
                   ((val << (8*3)) & 0x0000FF0000000000ull) |
                   ((val << (8*1)) & 0x000000FF00000000ull) |
                   ((val >> (8*1)) & 0x00000000FF000000ull) |
                   ((val >> (8*3)) & 0x0000000000FF0000ull) |
                   ((val >> (8*5)) & 0x000000000000FF00ull) |
                   ((val >> (8*7)) & 0x00000000000000FFull);
        }
        else
        {
            return val;
        }
    }
    private:
    bool _isLittleEndian;
};

#endif /* INFRA_H_ */
