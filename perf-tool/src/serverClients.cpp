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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "serverClients.hpp"
#include "infra.hpp"

#include <chrono>
#include <ctime>

using namespace std;

string NetworkClient::handlePoll()
{
    int n;
    char buffer[ServerConstants::BUFFER_SIZE];

    bzero(buffer, ServerConstants::BUFFER_SIZE);
    n = read(socketFd, buffer, ServerConstants::BUFFER_SIZE-1);

    if (n < 0)
    {
        perror("ERROR reading from socket");
    }
    else if (n > 0)
    {
        string s(buffer);
        s.pop_back();

        return s;
    }

    return "";
}

NetworkClient::NetworkClient(const int fd)
{
    socketFd = fd;
}

int NetworkClient::getSocketFd(void)
{
    return socketFd;
}

void NetworkClient::closeFd(void)
{
    close(socketFd);
    socketFd = -1;
}

LoggingClient::LoggingClient(const string filename)
{
    logFile = fopen(filename.c_str() ,"w");
    if (logFile == NULL)
    {
        fprintf(stderr, "Error opening  %s\n", filename.c_str());
    }
    else
    {
        fputs("[\n", logFile);
    }
}

void LoggingClient::closeFile(void)
{
    if (logFile)
    {
        /* Delete last comma to make a proper json array */
        long currentPos = ftell(logFile);
        fseek(logFile, currentPos - 2, SEEK_SET);
        fputs("\n]\n", logFile);
        
        fclose(logFile);
        logFile = NULL;
    }
}

void LoggingClient::logData(const json& message, std::string event, const std::string receivedFrom)
{
    if (logFile)
    {
        json log;
        auto currentClockTime = std::chrono::system_clock::now();
        long long int nano = std::chrono::duration_cast<std::chrono::nanoseconds>(currentClockTime.time_since_epoch()).count();
        log["body"] = message;
        log["from"] = receivedFrom;
        log["eventType"] = event;
        log["timestamp"] = nano;
        std::string s = log.dump(2, ' ', true);
        fputs(s.c_str(), logFile);
        fputs(",\n", logFile);
    }
}


