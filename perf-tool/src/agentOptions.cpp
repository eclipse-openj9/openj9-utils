#include <cstring>
#include <iostream>
#include <jvmti.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "agentOptions.hpp"
#include "infra.hpp"
#include "monitor.hpp"
#include "objectalloc.hpp"
#include "verboseLog.hpp"
#include "exception.hpp"
#include "methodEntry.hpp"


VerboseLogSubscriber *verboseLogSubscriber;

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
    jvmtiError error;
    if( !command.compare("stop")){
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.\n");
    } else if (!command.compare("start")) { // c == start
        error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable MonitorContendedEntered event notifications.\n");
    } else {
        invalidCommand(function, command);
    }
    return;
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

void modifyObjectAllocEvents(std::string function, std::string command, int sampleRate){
    jvmtiError error;
    setObjAllocSampleRate(sampleRate);
    if(!command.compare("stop")){
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable ObjectAlloc event.\n");
    } else if (!command.compare("start")) { // c == start
        error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable VM ObjectAlloc event notifications.\n");
    } else {
        invalidCommand(function, command);
    }
}


void modifyMethodEntryEvents(std::string function, std::string command, int sampleRate){
    jvmtiError error;
    if (sampleRate==0) {
        invalidRate(function, command, sampleRate);
        printf("\nmethodEntryEvents requires sampleRate > 0\n");
    } else {
        setMethodEntrySampleRate(sampleRate);
        if(!command.compare("stop")){
            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable MethodEntry event.\n");
        } else if (!command.compare("start")) { // c == start
            error = jvmti->SetEventNotificationMode(JVMTI_ENABLE,  JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable MethodEntry event notifications.\n");
        } else {
            invalidCommand(function, command);
        }
    }
}

void modifyVerboseLogSubscriber(std::string command, int sampleRate)
{
    if (!command.compare("start"))
    {
        verboseLogSubscriber = new VerboseLogSubscriber(jvmti);
        verboseLogSubscriber->setVerboseGCLogSampleRate(sampleRate);
        verboseLogSubscriber->Subscribe();
    } else if (!command.compare("stop"))
    {
        verboseLogSubscriber->Unsubscribe();
    }
}

void modifyExceptionBackTrace(std::string function, std::string command){
    // enable stack trace
    if (!command.compare("start")) {
        setExceptionBackTrace(true);
    } else if (!command.compare("stop")) {
        setExceptionBackTrace(false);
    } else {
        invalidCommand(function, command);
    }
}

void modifyExceptionEvents(std::string function, std::string command, int sampleRate){
    jvmtiError error;
    setExceptionSampleRate(sampleRate);

    if (!command.compare("start")) {
        error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to enable Exception event notifications.\n");
    } else if (!command.compare("stop")) {
        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
        check_jvmti_error(jvmti, error, "Unable to disable Exception event.\n");
    } else {
        invalidCommand(function, command);
    }
}

void agentCommand(json jCommand){
    jvmtiCapabilities capa;
    jvmtiError error;
    jvmtiPhase phase;

    std::string function;
    function = jCommand["functionality"].get<std::string>();
    std::string command;
    command = jCommand["command"].get<std::string>();
    int sampleRate = 1; // sampleRate is automatically set to 1. To turn off, set to 0
    if (jCommand.contains("sampleRate")) {
        sampleRate = jCommand["sampleRate"].get<int>();
        if (sampleRate <0 ) {
            invalidRate(function, command, sampleRate);
            printf("\nsampleRate must be >= 0\n");
        }
    }

    jvmti -> GetPhase(&phase);
    if(!(phase == JVMTI_PHASE_ONLOAD || phase == JVMTI_PHASE_LIVE)){
        check_jvmti_error(jvmti, JVMTI_ERROR_WRONG_PHASE, "AGENT CANNOT RECEIVE COMMANDS DURING THIS PHASE\n");
    } else {
        error = jvmti -> GetCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to get current capabilties.\n");

        if(!function.compare("monitorEvents")){
            modifyMonitorEvents(function, command);
        } else if(!function.compare("objectAllocEvents")){
            modifyObjectAllocEvents(function, command, sampleRate);
        } else if(!function.compare("monitorStackTrace")){
            modifyMonitorStackTrace(function, command);
        } else if(!function.compare("methodEntryEvents")){
            modifyMethodEntryEvents(function, command, sampleRate);
        } else if(!function.compare("verboseLog")){
            modifyVerboseLogSubscriber(command, sampleRate);
        } else if(!function.compare("exceptionEvents")){
            modifyExceptionEvents(function, command, sampleRate);
        } else {
            invalidFunction(function, command);
        }
    }
    return;
}
