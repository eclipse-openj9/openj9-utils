CC = g++
J = $$JAVA_HOME
CURRENT_DIR := $(shell pwd)

all: libagent.so

agent.o: agent.cpp
	$(CC) -I $(J)/include -I $(J)/include/linux/ -c -shared -fPIC -o $@ $^

infra.o: infra.cpp
	$(CC) -I $(CURRENT_DIR) -I $(J)/include -I $(J)/include/linux/ -c -shared -fPIC -o $@ $^

monitor.o: monitor.cpp
	$(CC) -I $(CURRENT_DIR) -I $(J)/include -I $(J)/include/linux/ -c -shared -fPIC -o $@ $^

server.o: server.cpp
	$(CC) -I $(CURRENT_DIR) -c -o $@ $^

libagent.so: agent.o infra.o monitor.o server.cpp
	$(CC) -I $(CURRENT_DIR) -I $(J)/include -I $(J)/include/linux/ -shared -fPIC -o $@ $^

clean:
	rm *.o


                                                                        