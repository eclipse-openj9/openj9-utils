#ifndef Agent_Options_H
#define Agent_Options_H
#include <string>
extern jvmtiEnv *jvmti;
extern JavaVM *javaVM;
void agentCommand(std::string f, std::string c);

#endif /* Agent_Options_H */