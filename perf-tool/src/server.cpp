#include "server.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <jvmti.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "perf.hpp"
#include "utils.hpp"
#include <fstream>
#include "AgentOptions.hpp"

using namespace std;
using json = nlohmann::json;

Server::Server(int portNo, string commandFileName, string logFileName)
{
    this->portNo = portNo;
    loggingClient = new LoggingClient(logFileName);
    
    if (commandFileName != "")
    {
        commandClient = new CommandClient(commandFileName);
    } 
    else 
    {
        headlessMode = false;
    }
}


void Server::handleServer()
{
    socklen_t clilen;
    char buffer[256], msg[256];
    string message;
    struct sockaddr_in serv_addr, cli_addr;
    int n, newsocketFd;

    // create a socket
    // socket(int domain, int type, int protocol)
    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd < 0)
    {
        error("ERROR opening socket");
    }

    // clear address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));

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
    if (bind(serverSocketFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to 5.
    listen(serverSocketFd, ServerConstants::NUM_CLIENTS);

    pollFds[0].fd = serverSocketFd;
    pollFds[0].events = POLLIN;
    pollFds[0].revents = 0;

    printf("Server started.\n");

    // Use polling to keep track of clients and keyboard input
    while (1)
    {
        if (!keepPolling) {
            break;
        }

        bzero(buffer, 256);

        if (poll(pollFds, activeNetworkClients + ServerConstants::BASE_POLLS, ServerConstants::POLL_INTERVALS) == -1)
        {
            error("ERROR on polling");
        }

        if (activeNetworkClients < ServerConstants::NUM_CLIENTS && pollFds[0].revents && POLLIN)
        {
            // The accept() call actually accepts an incoming connection
            clilen = sizeof(cli_addr);

            // This accept() function will write the connecting client's address info
            // into the the address structure and the size of that structure is clilen.
            // The accept() returns a new socket file descriptor for the accepted connection.
            // So, the original socket file descriptor can continue to be used
            // for accepting new connections while the new socker file descriptor is used for
            // communicating with the connected client.
            newsocketFd = accept(serverSocketFd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsocketFd < 0)
            {
                error("ERROR on accept");
            }
            else
            {
                networkClients[activeNetworkClients] = new NetworkClient(newsocketFd);
            }

            printf("server: got connection from %s port %d\n",
                   inet_ntoa(cli_addr.sin_addr),
                   ntohs(cli_addr.sin_port));

            // Send a welcome message
            sendMessage(newsocketFd, "Connection to server succeeded");

            // Update number of active clients
            activeNetworkClients++;
        }
        
        // Check for commands from commands file if running in headless mode
        if (headlessMode)
        {
            json command = commandClient->handlePoll();
            execCommand(command);
            handleClientCommand(command.dump(), "Commands file");
        }

        // Receiving and sending messages from/to clients
        for (int i = 0; i < activeNetworkClients; i++)
        {
            bzero(buffer, 256);
            string command = networkClients[i]->handlePoll(buffer);
            handleClientCommand(command, "Client");
        }

        handleMessagingClients();
    }
}


void Server::execCommand(json command){
    sleep(stoi((std::string) command["delay"]));
    if((command["functionality"].dump()).compare("perf")){
        agentCommand(command["functionality"].dump(), command["command"].dump());
    }
}


void Server::handleAgentData(string data)
{
    messageQueue.push(data);
    loggingClient->logData(data, "Agent");
}

void Server::handleClientCommand(string command, string from)
{
    if (command == "perf")
    {
        sendPerfDataToClient();
    }

    loggingClient->logData(command, from);
}

void Server::sendMessage(int socketFd, std::string message)
{
    const char *cstring = message.c_str();
    send(socketFd, cstring, strlen(cstring), 0);
}

void Server::handleMessagingClients()
{
    string message;

    while (!messageQueue.empty())
    {
        message = messageQueue.front();

        for (int i = 0; i < activeNetworkClients; i++)
        {   
            int clientSocketFd = networkClients[i]->getSocketFd();
            sendMessage(clientSocketFd, message);
        }
        loggingClient->logData(message, "Server");

        messageQueue.pop(); 
    }

}

void Server::sendPerfDataToClient(void)
{
    pid_t pid, currPid;
    json perfData;

    currPid = getpid();
    printf("pid: %d\n", currPid);
    // currPid = 44454;
    pid = fork();
    if (pid == -1)
    {
        perror("fork");
    }

    if (pid == 0)
    {
        perfData = perfProcess(currPid, 3);
        std::string perfStr;
        perfStr = perfData.dump();
        
        loggingClient->logData(perfStr, "perf");
        messageQueue.push(perfStr);
      
        exit(EXIT_SUCCESS);

    }// else parent continues on

}

void Server::shutDownServer()
{
    keepPolling = false;

    messageQueue.push("Server shutting down");
    messageQueue.push("done");   // keyword for clients to close their connection
    
    handleMessagingClients();

    close(serverSocketFd);
    
    loggingClient->closeFile();
    for (int i = 0; i < activeNetworkClients; i++)
    {   
        networkClients[i]->closeFd();
    }
    if (headlessMode) {
        commandClient->closeFile();
    }

    cout << "Server shutdown." << endl;
}
