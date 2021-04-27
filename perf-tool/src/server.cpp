/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "server.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <bits/types/time_t.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <jvmti.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "server.hpp"
#include "agentOptions.hpp"
#include "perf.hpp"
#include "utils.hpp"
#include "infra.hpp"

using namespace std;
using json = nlohmann::json;
std::mutex Server::serverMutex;

Server::Server(int portNo, const string &commandFileName, const string &logFileName)
{
    this->portNo = portNo;

    loggingClient = new LoggingClient(logFileName);
    loggingClient->logData("Server started", "serverStartEvent", "Server");

    if (commandFileName != "")
    {
        FILE *commandsFile = fopen(commandFileName.c_str() ,"r");
        if (commandsFile == NULL)
        {
            fprintf(stderr, "commands file %s doesn't exists\n", commandFileName.c_str());
            exit(0);
        }
        json commands;
        try
        {
            commands = json::parse(commandsFile);
        }
        catch (const std::exception& e)
        {
            fprintf(stderr, "%s\n Improper command received from: %s \n", e.what(), commandFileName.c_str());
            exit(0);
        }
        fclose(commandsFile);

        for (int i=0; i < commands.size(); i++)
        {
            /* Add this command to the ordered queue of commands */
            enqueueCommand(commands[i]);
            loggingClient->logData(commands[i], "commandDigestEvent", "Commands file");
        }
      
        /* Print commands */
        if (verbose >= Verbose::INFO)
        {
            printf("Commands from command file %s:\n", commandFileName.c_str());
            for (auto it : delayedCommands)
                printf("%s\n", it.command.dump().c_str());
        }
    }
    else
    {
        headlessMode = false;
    }
}

void Server::handleServer()
{
    socklen_t clilen;
    string message, command;
    struct sockaddr_in serv_addr, cli_addr;
    int n, newsocketFd;
    json jsonCommand;

    if (portNo != 0)
    {
        /* create a socket */
        /* socket(int domain, int type, int protocol) */
        serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocketFd < 0)
        {
            error("ERROR opening socket");
        }

        /* clear address structure */
        bzero((char *)&serv_addr, sizeof(serv_addr));

        /* setup the host_addr structure for use in bind call */
        /* server byte order */
        serv_addr.sin_family = AF_INET;

        /* automatically be filled with current host's IP address */
        serv_addr.sin_addr.s_addr = INADDR_ANY;

        /* convert short integer value for port must be converted into network byte order */
        serv_addr.sin_port = htons(portNo);

        /* This bind() call will bind  the socket to the current IP address on port, portNo */
        if (bind(serverSocketFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            error("ERROR on binding");
        }

        /* This listen() call tells the socket to listen to the incoming connections. */
        listen(serverSocketFd, ServerConstants::NUM_CLIENTS);

        pollFds[0].fd = serverSocketFd;
        pollFds[0].events = POLLIN;
        pollFds[0].revents = 0;
    }
    
    if (verbose >= Verbose::WARN)
        printf("Server started.\n");

    /* Use polling to keep track of network clients */
    if (serverSocketFd > 0)
    {
        while (keepPolling)
        {
            int pollTimeMillis = ServerConstants::POLL_INTERVALS;
            if (!delayedCommands.empty())
            {
                int nextOperationDelay = executeAllDueCommands();
                if (nextOperationDelay > 0)
                    pollTimeMillis = std::min(nextOperationDelay*1000, pollTimeMillis);
            }
            int pollResult = poll(pollFds, activeNetworkClients + ServerConstants::BASE_POLLS, pollTimeMillis);
            if (pollResult == -1)
            {
                perror("ERROR on polling, server will now shut down");
                break;
            }

            if (pollResult > 0) /* data is ready on at least one socket */
            {
                /* First check for incoming commands from network clients */
                for (int i = 0; i < activeNetworkClients; i++)
                {
                    int socketIndex = ServerConstants::BASE_POLLS + i;
                    if (pollFds[socketIndex].fd < 0)
                        continue; /* skip connections that have been closed */
                        
                    if (pollFds[socketIndex].revents & (POLLERR | POLLHUP | POLLNVAL))
                    {
                        /* Error encountered; we should close the socket */
                        if (pollFds[socketIndex].fd > 0)
                        {
                            networkClients[i]->closeFd();
                            pollFds[socketIndex].fd = -1; /* TODO: we need a linked list rather than an array */
                        }
                    }
                    else if (pollFds[ServerConstants::BASE_POLLS + activeNetworkClients].revents & POLLIN)
                    {
                        command = networkClients[i]->handlePoll();
                        if (!command.empty())
                        {
                            handleClientCommand(command, "Client");
                        }
                    }
                }
                /* Now, handle new connection requests */
                if (pollFds[0].revents & POLLIN)
                {
                    /* The accept() call actually accepts an incoming connection */
                    clilen = sizeof(cli_addr);

                    /* This accept() function will write the connecting client's address info 
                     * into the the address structure and the size of that structure is clilen.
                     */
                    newsocketFd = accept(serverSocketFd, (struct sockaddr *)&cli_addr, &clilen);
                    if (newsocketFd < 0)
                    {
                        perror("ERROR on accept");
                    }
                    else
                    {
                        /* Check the upper limit on active connections */
                        if (activeNetworkClients < ServerConstants::NUM_CLIENTS)
                        {
                            networkClients[activeNetworkClients] = new NetworkClient(newsocketFd);
                            if (verbose >= Verbose::INFO)
                                printf("server: got connection from %s port %d\n",
                                        inet_ntoa(cli_addr.sin_addr),
                                        ntohs(cli_addr.sin_port));

                            /* Send a welcome message */
                            sendMessage(newsocketFd, "Connection to server succeeded", "ServerStartEvent", "Server");

                            /* Prepare for poll */
                            int socketIndex = ServerConstants::BASE_POLLS + activeNetworkClients;
                            pollFds[socketIndex].fd = newsocketFd;
                            pollFds[socketIndex].events = POLLIN;
                            pollFds[socketIndex].revents = 0;

                            /* Update number of active clients */
                            activeNetworkClients++;
                        }
                        else /* Too many connections */
                        {
                            sendMessage(newsocketFd, "Connection refused. Too many connections", "ServerStartEvent", "Server");
                            close(newsocketFd); // TODO: send message to person that opened the socket
                            if (verbose >= Verbose::WARN)
                                printf("server refusing connection because there are too many\n");
                        }
                    }
                } 
            }
        }
    }
    else /* no network commands are allowed */
    {
        if (headlessMode && !delayedCommands.empty())
        {
            while(1) 
            {
                int nextOperationDelay = executeAllDueCommands();
                if (nextOperationDelay > 0)
                    std::this_thread::sleep_for(std::chrono::seconds(nextOperationDelay));
                else
                    break;
            } 
        }
    }
}

int Server::executeAllDueCommands()
{
    int nextOperationDelay = 0;
    std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    while (!delayedCommands.empty())
    {
        auto &firstEntry = delayedCommands.front();
        if (firstEntry.delayTill <= currentTime)
        {
            agentCommand(firstEntry.command);
            delayedCommands.pop_front();
        }
        else
        {
            /* find the time we need to schedule the next operation */
            nextOperationDelay = firstEntry.delayTill - currentTime;
            break;
        }
    }
    return nextOperationDelay; /* 0 means all the commands are finished */
}


void Server::enqueueCommand(json command)
{
    int delayValue = 0;
    if (command.find("delay") != command.end()) 
        {
        delayValue = command["delay"].get<int>();
        if (delayValue < 0)
            delayValue = 0;
        }
    std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::time_t delay_till = currentTime + delayValue;
    delayed_command_t dc(delay_till, command);
    /* Common case is to insert at the back */
    if (delayedCommands.empty() || delay_till >= delayedCommands.back().delayTill)
    {
        delayedCommands.push_back(dc);
    }
    else
    {
        /* Search the insertion point starting from back */
        auto rit = delayedCommands.rbegin();
        for (; rit != delayedCommands.rend() && rit->delayTill > delay_till; ++rit)
        {}
        delayedCommands.insert(rit.base(), dc);
    }
}


void Server::handleClientCommand(string command, string from)
{
    json com;
    try
    {
        com = json::parse(command);   
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "%s and Improper command received from: %s \n", e.what(), from.c_str());
        exit(0);
    }
    enqueueCommand(com);
    loggingClient->logData(com, "commandDigestEvent", from);
}


void Server::sendMessage(const int socketFd, const json& message, std::string event, const std::string receivedFrom)
{
    auto currentClockTime = std::chrono::system_clock::now();
    long long int nano = std::chrono::duration_cast<std::chrono::nanoseconds>(currentClockTime.time_since_epoch()).count();
    json log;
    log["body"] = message;
    log["from"] = receivedFrom;
    log["eventType"] = event;
    log["timestamp"] = nano;
    std::string result = log.dump(2, ' ', true);
    result += ",\n";
    int n, total = 0;
    size_t length = result.size();
    const char *buffer = result.data();

    while (total < length) 
    {
        n = send(socketFd, buffer, strlen(buffer), 0);
        if (n == -1) 
        {
            perror("ERROR sending message to clients failed"); 
        }

        total += n;
        buffer += n;
    }
}

void Server::handleMessagingClients(const json& message, std::string event)
{
    for (int i = 0; i < activeNetworkClients; i++)
    {
        int clientSocketFd = networkClients[i]->getSocketFd();
        sendMessage(clientSocketFd, message, event, "Server");
    }
    loggingClient->logData(message, event, "Server");
}

void Server::startPerfThread(int time)
{
    pid_t currPid;

    currPid = getpid();
    perfThread = thread(&perfProcess, currPid, time);
}

void Server::shutDownServer()
{
    keepPolling = false;

    /* wait on perf processing thread to join so its data can be sent before server closing */
    if (perfThread.joinable())
    {
        printf("Waiting on perf data\n");
        perfThread.join();
    }

    handleMessagingClients("Server shutting down", "ShutdownEvent");

    /* close off commands, logs, and network client sockets */
    for (int i = 0; i < activeNetworkClients; i++)
    {
        sendMessage(networkClients[i]->getSocketFd(), "done", "ShutdownEvent", "Server");
        networkClients[i]->closeFd();
        delete networkClients[i];
        networkClients[i] = NULL;
    }

    loggingClient->closeFile();
    delete loggingClient;
    loggingClient = NULL;

    close(serverSocketFd);
    serverSocketFd = -1;

    if (verbose >= Verbose::INFO)
        printf("Server shutdown.\n");
}
