#include <jvmti.h>
#include <iostream>
#include <stdio.h> 
#include <string.h>
#include <sys/types.h> 
#include <thread>
#include <unistd.h> 

#include "AgentOptions.hpp"
#include "infra.h"
#include "monitor.h"
#include "json.hpp"
#include "objectalloc.h"
#include "server.hpp"

using json = nlohmann::json;

jvmtiEnv *jvmti;
int portNo;
Server *server;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    std::string commandsPath = "";
    std::string logPath = "logs.txt";
    std::string token;
    std::string optionsDelim = ",";
    std::string pathDelim = ":";
    std::string oIn = (std::string) options;
    int pos1, pos2 = 0;
    portNo = 9002;
    if(!oIn.empty()){
        // there is a max of two options the user can supply here
        // "commands" is followed by a path to the commands file
        // "log" is followed by the path to the location for the log file
        // "portno" is followed by a number indicating the port to run the server on
        for(int i = 0; i < 3; i++){
            pos1 = oIn.find(optionsDelim);
            if(pos1 != std::string::npos){
                token = oIn.substr(0, pos1);
                pos2 = token.find(pathDelim);
                if(pos2 != std::string::npos){
                    if(!token.substr(0, pos2).compare("commands")){
                        token.erase(0, pos2 + pathDelim.length());
                        commandsPath = token;
                    }  else if(!token.substr(0, pos2).compare("log")){
                        token.erase(0, pos2 + pathDelim.length());
                        logPath = token;
                    } else if(!token.substr(0, pos2).compare("portno")){
                        token.erase(0, pos2 + pathDelim.length());
                        portNo = stoi(token);
                    }
                } else{
                    break;
                }
                oIn.erase(0, pos1 + optionsDelim.length());
            }
        }
    }

    std::cout << commandsPath << std::endl;
    std::cout << logPath << std::endl;
    std::cout << portNo << std::endl;

    server = new Server(portNo, commandsPath, logPath);
    
    jint rest = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (rest != JNI_OK || jvmti == NULL) {

        printf("Unable to get access to JVMTI version 1.0");
        return JNI_ERR;
    }

    jvmtiCapabilities capa;
    jvmtiError error;
    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    capa.can_signal_thread = 1;
    capa.can_get_owned_monitor_info = 1;
    capa.can_generate_monitor_events = 1;
    capa.can_generate_vm_object_alloc_events = 1;
    capa.can_generate_method_entry_events = 1;
    capa.can_tag_objects = 1;
    capa.can_get_current_thread_cpu_time	= 1;
    capa.can_get_line_numbers = 1;
    error = jvmti->AddCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Failed to set jvmtiCapabilities.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM init event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM death event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init MonitorContendedEntered event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE,  JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init Method Entry event.");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Unable to init VM Object Alloc event");

    jvmtiEventCallbacks callbacks;
    (void)memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
    callbacks.VMInit = &VMInit;
    callbacks.VMDeath = &VMDeath;
    callbacks.VMObjectAlloc = &VMObjectAlloc;
    callbacks.MonitorContendedEntered = &MonitorContendedEntered;
    callbacks.VMObjectAlloc = &VMObjectAlloc;
    callbacks.MethodEntry = &MethodEntry;
    error = jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    return JNI_OK;
}
