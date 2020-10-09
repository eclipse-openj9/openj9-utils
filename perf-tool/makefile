CC = g++
J = $$JAVA_HOME

all: libagent.so

agent.o: agent.cpp
	$(CC) -Wall -I $(J)/include -I $(J)/include/linux/ -I . -c -shared -fPIC -o $@ $^

infra.o: infra.cpp
	$(CC) -Wall -I $(J)/include -I $(J)/include/linux/ -I . -c -shared -fPIC -o $@ $^

server.o: server.cpp
	$(CC) -Wall -I . -c -shared -fPIC -pthread -o $@ $^

monitor.o: monitor.cpp
	$(CC) -Wall -I $(J)/include -I $(J)/include/linux/ -I . -c -shared -fPIC -o $@ $^

objectalloc.o: objectalloc.cpp
	$(CC) -Wall -I . -I $(J)/include -I $(J)/include/linux/ -c -shared -fPIC -o $@ $^

AgentOptions.o: AgentOptions.cpp
	$(CC) -Wall -I . -I $(J)/include -I $(J)/include/linux/ -c -shared -fPIC -o $@ $^

libagent.so: agent.o infra.o monitor.o objectalloc.o server.o AgentOptions.o
	$(CC) -Wall -I . -I $(J)/include -I $(J)/include/linux/ -shared -fPIC -o $@ $^

clean:
	rm -f *.o
	rm -f *.so


                                                                        
