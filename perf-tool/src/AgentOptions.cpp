#include <jvmti.h>
#include "objectalloc.h"
#include "infra.h"
#include <string.h>
#include "AgentOptions.hpp"
#include "monitor.h"
#include <unistd.h>
#include <stdio.h>

void agentCommand(functionality f, command c){
    jvmtiCapabilities capa;
    jvmtiError error;
    jvmtiPhase phase;

    sleep(1);

    jvmti -> GetPhase(&phase);
    if(!(phase == JVMTI_PHASE_ONLOAD || phase == JVMTI_PHASE_LIVE)){
        check_jvmti_error(jvmti, JVMTI_ERROR_WRONG_PHASE, "AGENT CANNOT RECEIVE COMMANDS DURING THIS PHASE!!!\n");
    } else{

        error = jvmti -> GetCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to get current capabilties");
        
        switch(f){

            case monitorEvents:
                if(capa.can_generate_monitor_events){
                    if( c == stop){
                        (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
                        capa.can_generate_monitor_events = 1;
                        printf("RELINQUISHING MONITOR EVENTS\n");

                        error = jvmti -> RelinquishCapabilities(&capa);
                        check_jvmti_error(jvmti, error, "Unable to relinquish \n");

                        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
                        check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.");
                    } else{ // c == start
                        printf("Monitor Events already disabled");
                    }
                } else{ // cannot generate monitor events
                    if(c == start){
                        (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
                        capa.can_generate_monitor_events = 1;
                        printf("ADDING MONITOR EVENT CAPABILITY");
                        
                        error = jvmti -> AddCapabilities(&capa);
                        check_jvmti_error(jvmti, error, "Unable to init monitor events capability");

                        error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
                        check_jvmti_error(jvmti, error, "Unable to enable MonitorContendedEntered event notifications.");
                    } else{
                        printf("Monitor Events already enabled");
                    }
                }
                break;
            case objectAllocEvents:
                if(capa.can_generate_vm_object_alloc_events){
                    if( c == stop){
                        (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
                        capa.can_generate_vm_object_alloc_events = 1;
                        printf("RELINQUISHING OBJECT ALLOC EVENTS\n");

                        error = jvmti -> RelinquishCapabilities(&capa);
                        check_jvmti_error(jvmti, error, "Unable to relinquish \n");

                        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
                        check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.");
                    } else{ // c == start
                    }
                }else{ // cannot generate monitor events
                    if(c == start){
                        (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
                        capa.can_generate_vm_object_alloc_events = 1;
                        printf("ADDING OBJECT ALLOC EVENT CAPABILITY");
                        
                        error = jvmti -> AddCapabilities(&capa);
                        check_jvmti_error(jvmti, error, "Unable to init object alloc events capability");

                        error = jvmti-> SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
                        check_jvmti_error(jvmti, error, "Unable to enable ObjectAlloc event notifications.");
                    } else{
                        printf("Monitor Events already enabled");
                    }
                }
                break;
        }
    } 
    return;
}