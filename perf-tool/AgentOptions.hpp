#ifndef Agent_Options_H
#define Agent_Options_H

enum functionality {monitorEvents, objectAllocEvents};
enum command {start, stop};

extern jvmtiEnv *jvmti;
void agentCommand(enum functionality f, enum command c);

#endif /* Agent_Options_H */