#ifndef agent_Options_H
#define agent_Options_H

#include "json.hpp"

using json = nlohmann::json;

extern jvmtiEnv *jvmti;
extern JavaVM *javaVM;
void agentCommand(json jCommand);


#endif /* agent_Options_H */