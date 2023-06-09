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

#ifndef SERVERCLIENTS_H_
#define SERVERCLIENTS_H_

#include <string>
#include <unistd.h>

#include "json.hpp"

using json = nlohmann::json;

class ServerConstants
{
public:
    static constexpr int NUM_CLIENTS = 5;
    static constexpr int BASE_POLLS = 1;
    static constexpr int POLL_INTERVALS = 500;
    static constexpr int COMMAND_INTERVALS = 500;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr int DEFAULT_PORT = 0;
};

class NetworkClient
{
    /*
     * Data members
     */
protected:
public:
private:
    int socketFd = 0;

    /*
     * Function members
     */
protected:
private:
public:
    NetworkClient(const int fd);

    int getSocketFd(void);
    void closeFd(void);
    std::string handlePoll();
};

class LoggingClient
{
    /*
     * Data members
     */
protected:
public:
private:
    FILE * logFile;

    /*
     * Function members
     */
protected:
private:
public:
    LoggingClient(const std::string filename);

    void closeFile(void);
    void logData(const json& data, std::string event, const std::string receivedFrom);
};

#endif
