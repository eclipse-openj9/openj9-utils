
#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#include <fstream>
#include "json.hpp"

#define NUM_CLIENTS 5
#define BASE_POLLS 1
#define POLL_INTERVAL 100
#define COMMAND_INTERVAL 500

using json = nlohmann::json; 

class NetworkClient {
public:
    int socketFd;

    NetworkClient(int fd) {
        socketFd = fd;
    }

    void sendMessage(std::string message);
    void handlePoll(char buffer[]);
};

class LoggingClient {
public:
    int socketFd;
    std::ofstream logFile;

    LoggingClient(std::string filename="logs.txt") {
        logFile.open(filename);
        if (!logFile.is_open()) {
            perror("ERROR opening logs file");
        }
    }

    void logData(std::string data, std::string recievedFrom);
};

class CommandClient {
public:
    int socketFd, commandInterval = 0;
    std::ifstream commandsFile;
    json commands;

    CommandClient(std::string filename="commands.txt") {
        commandsFile.open(filename);
        if (!commandsFile.is_open()) {
            perror("ERROR opening commands file");
        }
        try{
            commands = json::parse(commandsFile);
        }
        catch (...){
            // did not parse
        }
    }

    std::string handlePoll();
};

void passPathToCommandFile(std::string path);

void passPathToLogFile(std::string path);
void setServerPort(int val);

// Handle sending message to all clients.
void sendMessageToClients(std::string message);
void sendPerfDataToClient(void);

// Starts the server thread.
void JNICALL startServer(jvmtiEnv * jvmti, JNIEnv* jni, void* p);

// Handles server functionality
void handleServer();

// Write given data to log file
// Only writes to file if a log file was specified on server start
void logData(std::string data);

// Handles recieving data from agent
// Takes in the recieved data as char[]
void handleAgentData(const char *data);

// Handles client input for interactive mode
// Takes in the read buffer as char[]
void handleClientInput(char buffer[]);

// Join the server thread and closes server
void shutDownServer();

#endif /* SERVER_H_ */