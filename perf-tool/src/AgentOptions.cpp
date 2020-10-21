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


void agentCommand(std::string f, std::string c){
    jvmtiCapabilities capa;
    jvmtiError error;
    jvmtiPhase phase;
    jvmtiError* errorList;
    JNIEnv* env;

    jvmti -> GetPhase(&phase);
    if(!(phase == JVMTI_PHASE_ONLOAD || phase == JVMTI_PHASE_LIVE)){
        check_jvmti_error(jvmti, JVMTI_ERROR_WRONG_PHASE, "AGENT CANNOT RECEIVE COMMANDS DURING THIS PHASE\n");
    } else{

        error = jvmti -> GetCapabilities(&capa);
        check_jvmti_error(jvmti, error, "Unable to get current capabilties");
        
        if(!f.compare("monitorEvents")){
                if(capa.can_generate_monitor_events){
                    if( !c.compare("stop")){
                        (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
                        capa.can_generate_vm_object_alloc_events = 1;

                        error = jvmti -> RelinquishCapabilities(&capa);
                        check_jvmti_error(jvmti, error, "Unable to relinquish \n");
                        error = jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, (jthread)NULL);
                        check_jvmti_error(jvmti, error, "Unable to disable MonitorContendedEntered event.");
                    } else{ // c == start
                        printf("Monitor Events already disabled");
                    }
                } else{ // cannot generate monitor events
                    if(!c.compare("start")){
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
        } else if(!f.compare("objectAllocEvents")){
                if(capa.can_generate_vm_object_alloc_events){
                    if(!c.compare("stop")){
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
                    if(!c.compare("start")){
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
    }
    return;
}