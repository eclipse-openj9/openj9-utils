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

#ifndef SERVER_H_
#define SERVER_H_

#include <ctime>
#include <thread>
#include <poll.h>
#include <vector>
#include <mutex>

#include "serverClients.hpp"
#include "json.hpp"

using json = nlohmann::json;

struct delayed_command_t
{
    std::time_t delayTill;
    json command;
};

class Server
{
    /*
     * Data members
     */
protected:
public:
    static std::mutex serverMutex;
private:
    int serverSocketFd = -1, activeNetworkClients = 0, portNo;
    bool headlessMode = true, keepPolling = true;
    struct pollfd pollFds[ServerConstants::BASE_POLLS + ServerConstants::NUM_CLIENTS];
    NetworkClient *networkClients[ServerConstants::NUM_CLIENTS];
    CommandClient *commandClient;
    LoggingClient *loggingClient;
    std::thread perfThread;
    std::vector<delayed_command_t> delayedCommands;

    /*
     * Function members
     */
protected:
public:
    Server(int portNo, const std::string &commandFileName = "", const std::string &logFileName = "logs.txt");

    /* Handles server functionality and polling*/
    void handleServer(void);

    /* Handle the message queue, and sends to all clients */
    void handleMessagingClients(const json& message, std::string event);

    /* Closes all open files, connectend socketfd, and then the server's socket */
    void shutDownServer(void);

private:
    /* Handles recieving commands for the agent from clients */
    void handleClientCommand(const std::string command, const std::string from);

    void execCommand(json command);

    void sendMessage(const int socketFd, const json& message, std::string event, const std::string receivedFrom);

    void startPerfThread(int time);
};

#endif /* SERVER_H_ */
