#ifndef SERVERCLIENTS_H_
#define SERVERCLIENTS_H_

#include <string>
#include <fstream>
#include <unistd.h>

#include "json.hpp"

using json = nlohmann::json; 

class ServerConstants
{
public:
    static constexpr int NUM_CLIENTS = 5;
    static constexpr int BASE_POLLS = 1;
    static constexpr int POLL_INTERVALS = 100;
    static constexpr int COMMAND_INTERVALS = 500;
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
    NetworkClient(int fd)
    {
        socketFd = fd;
    }

    int getSocketFd(void)
    {
        return socketFd;
    }

    void closeFd(void)
    {
        close(socketFd);
    }

    void sendMessage(std::string message);
    std::string handlePoll(char buffer[]);
};


class CommandClient
{
    /*
     * Data members
     */
protected:
public:
private:
    int currentInterval = ServerConstants::COMMAND_INTERVALS;
    std::ifstream commandsFile;
    json commands;

    /*
     * Function members
     */
protected:
private:
    void execCommand(json command);

public:
    CommandClient(std::string filename);

    void closeFile(void)
    {
        commandsFile.close();
    }
    json handlePoll(void);
};


class LoggingClient
{
    /*
     * Data members
     */
protected:
public:
private:
    std::ofstream logFile;

    /*
     * Function members
     */
protected:
private:
public:
    LoggingClient(std::string filename);

    void closeFile(void)
    {
        logFile.close();
    }

    void logData(std::string data, std::string recievedFrom);
};

#endif