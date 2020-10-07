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

using namespace std;

int sockfd, activeNetworkClients = 0;
NetworkClient *networkClients[NUM_CLIENTS];
LoggingClient *loggingClient;
thread mainServerThread;
struct pollfd pollFds[BASE_POLLS+NUM_CLIENTS];
string logFileName;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void NetworkClient::sendMessage(std::string message) {
    const char *cstring = message.c_str();
    send(socketFd, cstring, strlen(cstring), 0);
}

void NetworkClient::handlePoll(char buffer[]) {
    int n = read(socketFd, buffer, 255);
    
    if (n < 0) { 
        error("ERROR reading from socket");
    } else if (n > 0) {
        // TODO handle client input
        printf("Client message recieved: %s\n", buffer);
    }
}

int main(int argc, char *argv[])
{
    int portNo;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    portNo = atoi(argv[1]);
    startServer(portNo);
    
    return 0; 
}

void startServer(int portNo, string filename) {
    mainServerThread = thread(handleServer, portNo);
    loggingClient = new LoggingClient();
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

        if (poll(pollFds, activeNetworkClients + BASE_POLLS, 100) == -1){
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

            // Update number of active clients
            activeNetworkClients++;

            // Send a welcome message
            networkClients[activeNetworkClients]->sendMessage("Connection to server succeeded");
        }

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

    // Fork new process to handle perf data retrieval
    if ((pid = fork()) < 0) {
	    error("Error on forking process"); 
    }

    currPid = getpid();
    if (pid == 0) {
        // Let child process handle getting perf data
        perfData = perfProcess(currPid); 
        std::string perfStr = perfData.dump();
        
        printf("Server has obtained perf data:\n%s\n", perfStr.c_str()); // for debugging
        
        // Relay data to client 
        sendMessageToClients(perfStr);
        
        // Log data to file
        logData(perfStr);

    } // else parent continues on 
}

void shutDownServer() {
    close(sockfd);
    for (int i=0; i<activeNetworkClients; i++) {
        close(networkClients[i]->socketFd);
    }

    mainServerThread.detach();
}
