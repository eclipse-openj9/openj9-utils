#include "server.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <jvmti.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "perf.hpp"
#include "utils.hpp"
#include <fstream>
#include "agentOptions.hpp"

using namespace std;
using json = nlohmann::json;

pid_t perfPid = -1;

Server::Server(int portNo, string commandFileName, string logFileName)
{
    this->portNo = portNo;

    if (commandFileName != "")
    {
        commandClient = new CommandClient(commandFileName);
    }
    else
    {
        headlessMode = false;
    }

    loggingClient = new LoggingClient(logFileName);
    loggingClient->logData("Server started", "Server");
}

void Server::handleServer()
{
    socklen_t clilen;
    char buffer[256], msg[256];
    string message, command;
    struct sockaddr_in serv_addr, cli_addr;
    int n, newsocketFd;
    json jsonCommand;

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

    // This bind() call will bind  the socket to the current IP address on port, portNo
    if (bind(serverSocketFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    // This listen() call tells the socket to listen to the incoming connections.
    listen(serverSocketFd, ServerConstants::NUM_CLIENTS);

    pollFds[0].fd = serverSocketFd;
    pollFds[0].events = POLLIN;
    pollFds[0].revents = 0;

    printf("Server started.\n");

    // Use polling to keep track of clients and keyboard input
    while (1)
    {
        if (!keepPolling)
        {
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
            jsonCommand = commandClient->handlePoll();
            if (!jsonCommand.empty())
            {
                command = jsonCommand.dump();
                handleClientCommand(command, "Commands file");
            }
        }

        // Receiving and sending messages from/to clients
        for (int i = 0; i < activeNetworkClients; i++)
        {
            bzero(buffer, 256);
            command = networkClients[i]->handlePoll(buffer);
            if (!command.empty())
            {
                handleClientCommand(command, "Client");
            }
        }

        handleMessagingClients();
    }
}

void Server::execCommand(json command)
{
    if (!command["delay"].is_null())
        sleep(command["delay"]);
    if ((command["functionality"].get<std::string>()).compare("perf"))
    {
        agentCommand(command["functionality"].get<std::string>(), command["command"].get<std::string>());
    }
    else
    {
        sendPerfDataToClient(command["time"]);
    }
}

void Server::handleClientCommand(string command, string from)
{
    json com;
    try
    {
        com = json::parse(command);
        execCommand(com);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::cerr << "Improper command received from: " << from << '\n';
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
    while (!messageQueue.empty())
    {
        for (int i = 0; i < activeNetworkClients; i++)
        {
            int clientSocketFd = networkClients[i]->getSocketFd();
            sendMessage(clientSocketFd, messageQueue.front());
        }
        loggingClient->logData(messageQueue.front(), "Agent");

        messageQueue.pop();
    }
}

void Server::sendPerfDataToClient(int time)
{
    pid_t currPid;
    json perfData;

    currPid = getpid();
    printf("pid: %d\n", currPid);
    perfPid = fork();
    if (perfPid == -1)
    {
        perror("fork");
    }

    if (perfPid == 0)
    {
        perfData = perfProcess(currPid, time);
        std::string perfStr;
        perfStr = perfData.dump();

        messageQueue.push(perfStr);
        loggingClient->logData(perfStr, "perf");

        exit(EXIT_SUCCESS);
    }
}

void Server::shutDownServer()
{
    keepPolling = false;

    if (perfPid != -1)
    {
        int status;
        cout << "Waiting on perf data." << endl;
        waitpid(perfPid, &status, WCONTINUED);
    }

    messageQueue.push("Server shutting down");
    messageQueue.push("done"); // keyword for clients to close their connection

    handleMessagingClients();
    close(serverSocketFd);

    loggingClient->closeFile();
    for (int i = 0; i < activeNetworkClients; i++)
    {
        networkClients[i]->closeFd();
    }
    if (headlessMode)
    {
        commandClient->closeFile();
    }

    cout << "Server shutdown." << endl;
}
