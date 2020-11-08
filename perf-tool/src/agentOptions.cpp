#include <jvmti.h>
#include "objectalloc.hpp"
#include "methodEntry.hpp"
#include "infra.hpp"
#include <string>
#include <cstring>
#include <jvmti.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "monitor.hpp"
#include "objectalloc.hpp"

void invalidCommand(std::string function, std::string command){
    printf("Invalid command with parameters: {functionality: %s, command: %s}\n", function.c_str(), command.c_str() );
}

void invalidFunction(std::string function, std::string command){
    printf("Invalid function with parameters: {functionality: %s, command: %s}\n", function.c_str(), command.c_str() );
}

void invalidRate(std::string function, std::string command, int sampleRate) {
    printf("Invalid rate with parameters: {functionality: %s, command: %s, sampleRate: %i}\n", function.c_str(), command.c_str(), sampleRate );
}

void modifyMonitorEvents(std::string function, std::string command){
    jvmtiCapabilities capa;
    jvmtiError error;

    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    error = jvmti -> GetCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Unable to get current capabilties");
    if(capa.can_generate_monitor_events){
        if( !command.compare("stop")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_monitor_events = 1;

            error = jvmti -> RelinquishCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to relinquish \n");
            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.");
        } else{ // c == start
            printf("Monitor Events already enabled\n");
        }
    } else{ // cannot generate monitor events
        if(!command.compare("start")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_monitor_events = 1;
            
            error = jvmti -> AddCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to init monitor events capability");

            error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable MonitorContendedEntered event notifications.");
        } else{ // c == stop
            printf("Monitor Events already disabled\n");
        }
    }
    return;
}

void modifyObjAllocBackTrace(std::string function, std::string command, int sampleRate){
    if (sampleRate==0){
        setObjAllocBackTrace(false);
    } else if (sampleRate>0){
        setObjAllocBackTrace(true);
        setObjAllocSampleRate(sampleRate);
    } else{
        invalidRate(function, command, sampleRate);
    }
}

void modifyObjectAllocEvents(std::string function, std::string command, int sampleRate){
    jvmtiCapabilities capa;
    jvmtiError error;

    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    error = jvmti -> GetCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Unable to get current capabilties");
    modifyObjAllocBackTrace(function, command, sampleRate);
    if(capa.can_generate_vm_object_alloc_events){
        if(!command.compare("stop")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_vm_object_alloc_events = 1;

            error = jvmti -> RelinquishCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to relinquish \n");

            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable ObjectAlloc event.");
        } else if (!command.compare("start")) { // c == start
            printf("Object Alloc Events already enabled");
        } else {
            invalidCommand(function, command);
        }
    }else{ // cannot generate monitor events
        if(!command.compare("start")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_vm_object_alloc_events = 1;
            
            error = jvmti -> AddCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to init object alloc events capability");

            error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable VM ObjectAlloc event notifications.");
        } else if (!command.compare("stop")) { // c == stop
            printf("Obect Alloc Events already disabled");
        } else {
            invalidCommand(function, command);
        }
    }

}

void modifyMonitorStackTrace(std::string function, std::string command){
    // enable stack trace 
    if (!command.compare("start")){
        setMonitorStackTrace(true);
    } else if (!command.compare("stop")){
        setMonitorStackTrace(false);
    } else{
        invalidCommand(function, command);
    }
}

void modifyMethodEntryEvents(std::string function, std::string command, int sampleRate){
    jvmtiCapabilities capa;
    jvmtiError error;

    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    error = jvmti -> GetCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Unable to get current capabilties\n");
    setMethodEntrySamplingRate(sampleRate);
    if(capa.can_generate_method_entry_events){
        if(!command.compare("stop")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            
            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable MethodEntry event.\n");
        } else if (!command.compare("start")){ // c == start
        printf("Method Entry Events already enabled\n");
        } else {
            invalidCommand(function, command);
        }
    }else{ // cannot generate method entry events
        if(!command.compare("start")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));

            error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable MethodEntry event notifications.\n");
        } else if (!command.compare("stop")){ // c == stop
            printf("Method Entry Events already disabled\n");
        } else {
            invalidCommand(function, command);
        }
    }

}



void agentCommand(std::string function, std::string command, int sampleRate){
    jvmtiCapabilities capa;
    jvmtiError error;
    jvmtiPhase phase;

    jvmti -> GetPhase(&phase);
    if(!(phase == JVMTI_PHASE_ONLOAD || phase == JVMTI_PHASE_LIVE)){
        check_jvmti_error(jvmti, JVMTI_ERROR_WRONG_PHASE, "AGENT CANNOT RECEIVE COMMANDS DURING THIS PHASE\n");
    } else{
         error = jvmti -> GetCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to get current capabilties");        
        if(!function.compare("monitorEvents")){
            modifyMonitorEvents(function, command);
        } else if(!function.compare("objectAllocEvents")){
            modifyObjectAllocEvents(function, command, sampleRate);
        } else if(!function.compare("monitorStackTrace")){
            modifyMonitorStackTrace(function, command);
        } else if(!function.compare("methodEntryEvents")){
            modifyMethodEntryEvents(function, command, sampleRate);
        } else {
            invalidFunction(function, command);
        }
    }
    return;
}