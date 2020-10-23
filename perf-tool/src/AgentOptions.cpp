#include <jvmti.h>
#include "objectalloc.h"
#include "infra.h"
#include <string>
#include <cstring>
#include "AgentOptions.hpp"
#include "monitor.h"
#include <unistd.h>
#include <stdio.h>
#include <iostream>


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
            printf("Monitor Events already disabled");
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
            printf("Monitor Events already enabled");
        }
    }
    return;
}


void modifyObjectAllocEvents(std::string function, std::string command){
    jvmtiCapabilities capa;
    jvmtiError error;

    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    error = jvmti -> GetCapabilities(&capa);
    check_jvmti_error(jvmti, error, "Unable to get current capabilties");
    if(capa.can_generate_vm_object_alloc_events){
        if(!command.compare("stop")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_vm_object_alloc_events = 1;

            error = jvmti -> RelinquishCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to relinquish \n");

            error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.");
        } else{ // c == start
        printf("Object Alloc Events already disabled");
        }
    }else{ // cannot generate monitor events
        if(!command.compare("start")){
            (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
            capa.can_generate_vm_object_alloc_events = 1;
            
            error = jvmti -> AddCapabilities(&capa);
            check_jvmti_error(jvmti, error, "Unable to init object alloc events capability");

            error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
            check_jvmti_error(jvmti, error, "Unable to enable ObjectAlloc event notifications.");
        } else{ // c == stop
            printf("Obect Alloc Events already enabled");
        }
    }

}



void agentCommand(std::string function, std::string command){
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
            modifyObjectAllocEvents(function, command);    
        }
    }
    return;
}
