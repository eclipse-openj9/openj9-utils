#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int sockfd, newsockfd;

// Handle sending message to all clients.
void sendMessageToClients(string message);

// Starts the server and sends a connection succeeded message.
void startServer(int portNo);
