CC = g++
J = $$JAVA_HOME

all: libagent.so

agent.o: agent.cpp
	$(CC) -I $(J)/include -I $(J)/include/linux/ -c -shared -fPIC -o $@ $^

infra.o: infra.cpp
	$(CC) -I $(J)/include -I $(J)/include/linux/ -I . -c -shared -fPIC -o $@ $^

monitor.o: monitor.cpp
	$(CC) -I $(J)/include -I $(J)/include/linux/ -I . -c -shared -fPIC -o $@ $^

libagent.so: agent.o infra.o monitor.o
	$(CC) -I $(J)/include -I $(J)/include/linux/ -shared -fPIC -o $@ $^

clean:
	rm -f *.o
	rm -f *.so


                                                                        
