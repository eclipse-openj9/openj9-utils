#include "serverClients.hpp"

#include <chrono>
#include <ctime>

using namespace std;

string NetworkClient::handlePoll(char buffer[])
{
    int n = read(socketFd, buffer, 255);
    string s = string(buffer);
    s.pop_back();

    if (n < 0)
    {
        perror("ERROR reading from socket");
    }
    else if (n > 0)
    {
        return s;
    }

    return "";
}

NetworkClient::NetworkClient(int fd)
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
}

LoggingClient::LoggingClient(string filename)
{
    logFile.open(filename);
    if (!logFile.is_open())
    {
        perror("ERROR opening logs file");
    }
    else
    {
        logFile << "[" << endl;
    }
}

void LoggingClient::closeFile(void)
{
    if (logFile.is_open())
    {
        logFile << "]" << endl;
        logFile.close();
    }
}

void LoggingClient::logData(string message, string recievedFrom)
{
    if (logFile.is_open())
    {
        json log;
        auto currentClockTime = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(currentClockTime);
        
        // If message was a proper json, log as such
        // Otherwise log the string
        try
        {
            log["body"] = log.parse(message);
        }
        catch(const std::exception& e)
        {
            log["body"] = message;
        }
        
        log["from"] = recievedFrom;
        log["timestamp"] = std::ctime(&currentTime);
        logFile << log.dump(2, ' ', true) << ',' << endl;
    }
}

CommandClient::CommandClient(std::string filename)
{
    commandsFile.open(filename);
    if (!commandsFile.is_open())
    {
        printf("filename: %s\n", filename.c_str());
        perror("ERROR opening commands file");
    }

    commands = json::parse(commandsFile);
}

void CommandClient::closeFile(void)
{
    if (commandsFile.is_open())
    {
        commandsFile.close();
    }
}

json CommandClient::handlePoll()
{
    static int commandNumber = 0;
    static const int numCommands = commands.size();
    json j;

    if (currentInterval <= 0)
    {
        if (commandNumber < numCommands)
        {
            commandNumber++;
            return commands[commandNumber];
        }
        currentInterval = ServerConstants::COMMAND_INTERVALS;
    }
    else
    {
        currentInterval = currentInterval - ServerConstants::POLL_INTERVALS;
    }

    return j;
}