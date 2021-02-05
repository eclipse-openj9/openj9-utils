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

#include "client.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "serverClients.hpp"
#include "utils.hpp"

#define POLL_INTERVAL 150
#define BUFFER_SIZE 4096

using namespace std;

Client::Client(int _portno, const string _hostname, bool _interactive_mode)
{
    portno = _portno;
    hostname = _hostname;
    interactive_mode = _interactive_mode;
}

void Client::openServerConnection()
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        error("ERROR opening socket");
    }

    server = gethostbyname(hostname.c_str());
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(socketFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }
}

void Client::startClient()
{

    openServerConnection();

    if (interactive_mode)
    {
        pollFds[0].fd = STDIN_FILENO;
        pollFds[0].events = POLLIN;
        pollFds[0].revents = 0;
        printf("Enter message to send to server: \n");
    }

    pollFds[1].fd = socketFd;
    pollFds[1].events = POLLIN;
    pollFds[1].revents = 0;

    handlePolling();
}

void Client::handlePolling()
{
    char buffer[BUFFER_SIZE];

    while (keepPolling)
    {
        bzero(buffer, BUFFER_SIZE);

        if (poll(pollFds, 2, POLL_INTERVAL) == -1)
        {
            error("ERROR on polling");
        }

        if (interactive_mode && pollFds[0].revents && POLLIN)
        {
            fgets(buffer, BUFFER_SIZE, stdin);
            sendMessage(buffer);
        }

        if (pollFds[1].revents && POLLIN)
        {
            bzero(buffer, BUFFER_SIZE);
            receiveMessage(buffer);
        }
    }

    closeClient();
}

void Client::sendMessage(const char message[])
{
    int n;
    n = write(socketFd, message, strlen(message));
    if (n < 0)
    {
        error("ERROR writing to socket");
    }
}

void Client::receiveMessage(char buffer[])
{
    int n;

    n = recv(socketFd, buffer, BUFFER_SIZE-1, MSG_DONTWAIT);
    if (n < 0)
    {
        error("ERROR reading from socket");
    }

    if (n > 0)
    {
        printf("Received: %s\n", buffer);

        if (string(buffer).substr(strlen(buffer) - 4, 4) == "done")
        {
            keepPolling = false;
        }
    }
}

void Client::closeClient()
{
    keepPolling = false;
    close(socketFd);
    printf("Closed connection with server.\n");
}

int main(int argc, char const *argv[])
{
    string hostname = "localhost";
    int portno = ServerConstants::DEFAULT_PORT;

    if (argc > 1)
    {
        portno = atoi(argv[1]);
        if (argc > 2)
            hostname = argv[2];
    }

    Client client(portno, hostname, true);

    client.startClient();

    return 0;
}
