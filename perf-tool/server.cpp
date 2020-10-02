/* The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <server.hpp>
#include <perf.hpp>
#include <thread>
#include <poll.h>

using namespace std;

int sockfd, clientFds[NUM_CLIENTS], activeClients = 0;
thread mainServerThread;
struct pollfd pollFds[BASE_POLLS+NUM_CLIENTS];
string logFileName;
ofstream logFile;

void error(const char *msg) {
    perror(msg);
    exit(1);
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
    logFile.open("out.txt");
    if (filename != "") {
        logFile.open(filename);
        if (!logFile.is_open()) {
            error("ERROR opening logs file");
        }
    }
}

void handleServer(int portNo) {
    socklen_t clilen;
    char buffer[256], msg[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

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

        if (poll(pollFds, activeClients + BASE_POLLS, 0) == -1){
            error("ERROR on polling");
        }

        if (activeClients < NUM_CLIENTS && pollFds[0].revents && POLLIN) {
            // The accept() call actually accepts an incoming connection
            clilen = sizeof(cli_addr);

            // This accept() function will write the connecting client's address info 
            // into the the address structure and the size of that structure is clilen.
            // The accept() returns a new socket file descriptor for the accepted connection.
            // So, the original socket file descriptor can continue to be used 
            // for accepting new connections while the new socker file descriptor is used for
            // communicating with the connected client.
            clientFds[BASE_POLLS+activeClients] = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (clientFds[BASE_POLLS+activeClients] < 0) { 
                error("ERROR on accept");
            }

            printf("server: got connection from %s port %d\n",
                inet_ntoa(cli_addr.sin_addr), 
                ntohs(cli_addr.sin_port)
            );

            // Update number of active clients
            activeClients++;

            // Send a welcome message
            sendMessageToClients("Connection to server succeeded");
        }

        // Receiving and sending messages from/to clients 
        for (int i=0; i<activeClients; i++) {
            bzero(buffer,256);
            n = read(clientFds[i+BASE_POLLS], buffer, 255);
            
            if (n < 0) { 
                error("ERROR reading from socket");
            } else if (n > 0) {
                handleClientInput(buffer);
            }
        }
    }

}

void logData(string data) {
    if (logFile.is_open()) {
        logFile << data << endl;
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
    const char *cstring = message.c_str();
    for (int i=0; i<activeClients; i++) {
        send(clientFds[i+BASE_POLLS], cstring, strlen(cstring), 0);
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
    for (int i=0; i<activeClients; i++) {
        close(clientFds[i]);
    }

    mainServerThread.detach();
}
