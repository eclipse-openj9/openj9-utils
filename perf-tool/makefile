CC = g++
J = $$JAVA_HOME
libagent.so: agent.cpp infra.cpp monitor.cpp
	$(CC) -I $(J)/include -I $(J)/include/linux/ -shared -fPIC -o $@ $^
                                                                        