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
#include <getopt.h>
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

Client::Client(int _portno, const string _clientLogPath, const string _hostname, bool _interactive_mode)
{
    portno = _portno;
    clientLogPath = _clientLogPath;
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

void Client::openFile()
{
    clientLogFile = fopen(clientLogPath.c_str() ,"a");
    if (clientLogFile == NULL)
        fprintf(stderr, "Error opening  %s\n", clientLogPath.c_str());
    else
        fputs("[\n", clientLogFile);
}

void Client::closeFile()
{
     if (clientLogFile)
     {
         /* Delete last comma to make a proper json array */
         long currentPos = ftell(clientLogFile);
         fseek(clientLogFile, currentPos - 2, SEEK_SET);
         fputs("\n]\n", clientLogFile);
         fclose(clientLogFile);
     }
}

void Client::startClient()
{

    openServerConnection();
    openFile();

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

    closeFile();
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
        fputs(buffer, clientLogFile);

        if (string(buffer).find("done") != string::npos)
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
void printUsage()
{
    printf("Usage: ./a.out [OPTIONS]\n\n"
            "Options:\n"
            "--p, --portNo=Port          Specify port number to connect to server\n"
            "--l, --log=logname          Specify Output file name, default log is clientLogs.json\n"
            "--h, --host=hostname        Specify host, default is localhost\n"
            "--u, --usage                shows usage\n\n"
            "Eg: ./a.out --portNo=9002 --log=log.json --host=localhost\n"
            "(OR) Use Short_Options: ./a.out --p=9002 --l=log.json --h=localhost\n");
}

int main(int argc, char *argv[])
{
    string hostname = "localhost";
    std::string clientLogPath = "clientLogs.json";
    int portno = ServerConstants::DEFAULT_PORT;
    static struct option long_options[] = {
        {"portNo", optional_argument, 0, 'p'},
        {"host", optional_argument, 0, 'h'},
        {"log", optional_argument, 0, 'l'},
        {"usage", no_argument, 0, 'u'},
        {0, 0, 0, 0}
    };
    int c, index;
    while (true) {
        c = getopt_long(argc, argv, ":hlpu", long_options, &index);
        if (c == -1)
            break;
        switch (c) {
            case 'h':
                if (optarg)
                    hostname = optarg;
                else
                    printf("No argument passed to hostname using default\n");
                break;

            case 'l':
                if (optarg)
                    clientLogPath = optarg;
                else
                    printf("No argument passed to log using default\n");
                break;

            case 'p':
                if (optarg)
                {
                    portno = atoi(optarg);
                }
                else
                {
                    printf("specify portNo to connect to server\n");
                    exit(0);
                }
                break;

            case 'u':
                printUsage();
                break;

            case '?':
                printf("Unknown Option\n");
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
                break;
        }
    }    
    Client client(portno, clientLogPath, hostname, true);

    client.startClient();

    return 0;
}
