/* The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <server.hpp>
#include <perf.hpp>
#include <thread>
#include <poll.h>
#include "utils.hpp"

using namespace std;

int sockfd, activeNetworkClients = 0;
NetworkClient *networkClients[NUM_CLIENTS];
LoggingClient *loggingClient;
CommandClient *commandClient;
thread mainServerThread;
struct pollfd pollFds[BASE_POLLS+NUM_CLIENTS];
string logFileName;

void NetworkClient::sendMessage(std::string message) {
    const char *cstring = message.c_str();
    send(socketFd, cstring, strlen(cstring), 0);
}

void NetworkClient::handlePoll(char buffer[]) {
    int n = read(socketFd, buffer, 255);
    string s = string(buffer);
    s.pop_back();
    
    if (n < 0) { 
        error("ERROR reading from socket");
    } else if (n > 0) {
        // TODO handle client input

        loggingClient->logData(s, "Client");
    }
}

void LoggingClient::logData(string message, string recievedFrom) {
    if (logFile.is_open()) {
        logFile << "recieved: " << message << " | from: " << recievedFrom << endl;
    }
}

string CommandClient::handlePoll() {
    if (commandInterval <= 0) {
        getline(commandsFile, currentLine);
        if (!commandsFile.eof()) {
            loggingClient->logData(currentLine, "Command File");
            commandInterval = COMMAND_INTERVAL;
        } 
    } else {
        commandInterval = commandInterval - POLL_INTERVAL;
    }

    return currentLine;
}

void startServer(int portNo, string filename) {
    mainServerThread = thread(handleServer, portNo);
    loggingClient = new LoggingClient();
    commandClient = new CommandClient();
}

void handleServer(int portNo) {
    socklen_t clilen;
    char buffer[256], msg[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n, newsocketFd;

    // create a socket
    // socket(int domain, int type, int protocol)
    sockfd =  socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    error("ERROR opening socket");

    // clear address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));

    /* setup the host_addr structure for use in bind call */
    // server byte order
    serv_addr.sin_family = AF_INET;

    // automatically be filled with current host's IP address
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // convert short integer value for port must be converted into network byte order
    serv_addr.sin_port = htons(portNo);

    // bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    // bind() passes file descriptor, the address structure,
    // and the length of the address structure
    // This bind() call will bind  the socket to the current IP address on port, portNo
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to 5.
    listen(sockfd,NUM_CLIENTS);

    pollFds[0].fd = sockfd;
    pollFds[0].events = POLLIN;
    pollFds[0].revents = 0;

    printf("Server started.\n");

    // Use polling to keep track of clients and keyboard input
    while (1) {
        bzero(buffer,256);

        if (poll(pollFds, activeNetworkClients + BASE_POLLS, POLL_INTERVAL) == -1){
            error("ERROR on polling");
        }

        if (activeNetworkClients < NUM_CLIENTS && pollFds[0].revents && POLLIN) {
            // The accept() call actually accepts an incoming connection
            clilen = sizeof(cli_addr);

            // This accept() function will write the connecting client's address info 
            // into the the address structure and the size of that structure is clilen.
            // The accept() returns a new socket file descriptor for the accepted connection.
            // So, the original socket file descriptor can continue to be used 
            // for accepting new connections while the new socker file descriptor is used for
            // communicating with the connected client.
            newsocketFd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsocketFd < 0) { 
                error("ERROR on accept");
            } else {
                networkClients[activeNetworkClients] = new NetworkClient(newsocketFd);
            }

            printf("server: got connection from %s port %d\n",
                inet_ntoa(cli_addr.sin_addr), 
                ntohs(cli_addr.sin_port)
            );

            // Send a welcome message
            networkClients[activeNetworkClients]->sendMessage("Connection to server succeeded");

            // Update number of active clients
            activeNetworkClients++;
        }

        // Check for commands from commands file
        commandClient->handlePoll();

        // Receiving and sending messages from/to clients 
        for (int i=0; i<activeNetworkClients; i++) {
            bzero(buffer,256);
            networkClients[i]->handlePoll(buffer);
        }
    }

}

void handleAgentData(const char *data) {
    if (data == "perf\n") {
        sendPerfDataToClient();
    }
    string s = string(data);
    logData(s); 
    printf("Recieved: %s\n", data);
}

void handleClientInput(char buffer[]) {
    string s = string(buffer);
    logData(s); 
    printf("%s\n", buffer);
}

void sendMessageToClients(string message) {
    for (int i=0; i<activeNetworkClients; i++) {
        networkClients[i]->sendMessage(message);
    }
}

void sendPerfDataToClient(void) {
    pid_t pid, currPid;
    json perfData;

    currPid = getpid();
    // currPid = 44454;
    pid = fork();
    if (pid == -1)
      perror("fork");

    if (pid == 0) {
      perfData = perfProcess(currPid, 3);
      std::string perfStr;
      perfStr = perfData.dump();

      sendMessageToClients(perfStr.c_str()); // for debugging

      // Relay data to client
      sendMessageToClients("done");
    } // else parent continues on

}

void shutDownServer() {
    close(sockfd);
    for (int i=0; i<activeNetworkClients; i++) {
        close(networkClients[i]->socketFd);
    }

    loggingClient->logFile.close();
    commandClient->commandsFile.close();

    mainServerThread.detach();
}
