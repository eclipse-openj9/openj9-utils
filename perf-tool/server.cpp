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

int sockfd, newsockfd;

using namespace std;

void error(const char *msg)
{
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

void sendMessageToClients(string message) {
    const char *cstring = message.c_str();
    send(newsockfd, cstring, strlen(cstring), 0);
}

void startServer(int portNo) {
    socklen_t clilen;
    char buffer[256], msg[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    printf("Reached here.\n");

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
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to 5.
    listen(sockfd,5);

    // The accept() call actually accepts an incoming connection
    clilen = sizeof(cli_addr);

    // This accept() function will write the connecting client's address info 
    // into the the address structure and the size of that structure is clilen.
    // The accept() returns a new socket file descriptor for the accepted connection.
    // So, the original socket file descriptor can continue to be used 
    // for accepting new connections while the new socker file descriptor is used for
    // communicating with the connected client.
    newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
        error("ERROR on accept");

    printf("server: got connection from %s port %d\n",
        inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    // This send() function sends the bytes of the string to the new socket
    sendMessageToClients("Connection to server succeeded");

    bzero(buffer,256);

    n = read(newsockfd,buffer,255);

    if (strcmp(buffer, "perf\n") == 0) {
	sendPerfDataToClient();
    }
	
    if (n < 0) error("ERROR reading from socket");
    printf("%s\n", buffer);

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
    } // else parent continues on 
}

void shutDownServer() {
    close(newsockfd);
    close(sockfd);
}
