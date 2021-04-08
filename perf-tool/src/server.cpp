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

    if (commandFileName != "")
    {
        commandClient = new CommandClient(commandFileName);
    }
    else
    {
        headlessMode = false;
    }

    loggingClient = new LoggingClient(logFileName);
    loggingClient->logData("Server started", "serverStartEvent", "Server");
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
    
    if (verbose >= WARN)
        printf("Server started.\n");

    /* Use polling to keep track of clients and keyboard input */
    while (keepPolling)
    {
        if (serverSocketFd >= 0)
        {
            if (poll(pollFds, activeNetworkClients + ServerConstants::BASE_POLLS, ServerConstants::POLL_INTERVALS) == -1)
            {
                perror("ERROR on polling, server will now shut down");
                break;
            }

            if (activeNetworkClients < ServerConstants::NUM_CLIENTS && (pollFds[0].revents & POLLIN))
            {
                /* The accept() call actually accepts an incoming connection */
                clilen = sizeof(cli_addr);

                /* This accept() function will write the connecting client's address info 
                 *into the the address structure and the size of that structure is clilen.
                 */
                newsocketFd = accept(serverSocketFd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsocketFd < 0)
                    perror("ERROR on accept");
                else
                    networkClients[activeNetworkClients] = new NetworkClient(newsocketFd);
                
                if (verbose == INFO)
                    printf("server: got connection from %s port %d\n",
                       inet_ntoa(cli_addr.sin_addr),
                       ntohs(cli_addr.sin_port));

                /* Send a welcome message */
                sendMessage(newsocketFd, "Connection to server succeeded", "ServerStartEvent", "Server");

                /* Update number of active clients */
                activeNetworkClients++;
            }
        }
        /* Check for commands from commands file if running in headless mode */
        if (headlessMode)
        {
            jsonCommand = commandClient->handlePoll();
            if (!jsonCommand.empty())
            {
                command = jsonCommand.dump();
                handleClientCommand(command, "Commands file");
            }
        }

        /* Receiving messages to clients */
        for (int i = 0; i < activeNetworkClients; i++)
        {
            command = networkClients[i]->handlePoll();
            if (!command.empty())
            {
                handleClientCommand(command, "Client");
            }
        }

        /* Checks if it is the time for any delayed command to be fired */
        if (!delayedCommands.empty()) {
            auto currentClockTime = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(currentClockTime);

            while(delayedCommands.size() > 0)
            {
                if (delayedCommands[0].delayTill <= currentTime) {
                    agentCommand(delayedCommands[0].command);
                    delayedCommands.erase(delayedCommands.begin());
                } else{
                    break;
                }
            }
        }
    }
}

void Server::execCommand(json command)
{
    if (command.find("delay") != command.end() && command["delay"].get<int>() > 0) {
        auto currentClockTime = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(currentClockTime);
        std::time_t delay_till = currentTime + command["delay"].get<int>();

        delayed_command_t delayedCommand;
        delayedCommand.command = command;
        delayedCommand.delayTill = delay_till;
        delayedCommands.push_back(delayedCommand);

        sort(delayedCommands.begin(), delayedCommands.end(),
            [](delayed_command_t a, delayed_command_t b) {
                if (a.delayTill < b.delayTill) {
                    return true;
                } else {
                    return false;
                }
        });
    }
    else if ((command["functionality"].get<std::string>()).compare("perf"))
    {
        agentCommand(command);
    }
    else
    {
        startPerfThread(command["time"]);
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
        fprintf(stderr, "%s and Improper command received from: %s \n", e.what(), from.c_str());
        exit(0);
    }

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
        cout << "Waiting on perf data." << endl;
        perfThread.join();
    }

    handleMessagingClients("Server shutting down", "");

    /* close off commands, logs, and network client sockets */
    for (int i = 0; i < activeNetworkClients; i++)
    {
        sendMessage(networkClients[i]->getSocketFd(), "done", "ShutdownEvent", "Server");
        networkClients[i]->closeFd();
        delete networkClients[i];
    }

    if (headlessMode)
    {
        commandClient->closeFile();
        delete commandClient;
    }

    loggingClient->closeFile();
    delete loggingClient;

    close(serverSocketFd);

    cout << "Server shutdown." << endl;
}
