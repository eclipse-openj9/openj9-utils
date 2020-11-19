
#ifndef SERVER_H_
#define SERVER_H_

#include <thread>
#include <queue>
#include <poll.h>

#include "serverClients.hpp"
#include "json.hpp"

using json = nlohmann::json;

class Server
{
    /*
     * Data members
     */
protected:
public:
    std::queue<std::string> messageQueue;

private:
    int serverSocketFd, activeNetworkClients = 0, portNo;
    bool headlessMode = true, keepPolling = true;
    std::thread mainServerThread;
    struct pollfd pollFds[ServerConstants::BASE_POLLS + ServerConstants::NUM_CLIENTS];
    NetworkClient *networkClients[ServerConstants::NUM_CLIENTS];
    CommandClient *commandClient;
    LoggingClient *loggingClient;

    /*
     * Function members
     */
protected:
public:
    Server(int portNo, std::string commandFileName = "", std::string logFileName = "logs.txt");

    /* Handles server functionality and polling*/
    void handleServer();

    /* Handle the message queue, and sends to all clients */
    void handleMessagingClients();

    /* Join the server thread and closes server's socket */
    void shutDownServer(void);

    void execCommand(json command);

private:
    /* Handles recieving commands for the agent from clients */
    void handleClientCommand(const std::string command, const std::string from);

    void sendMessage(const int socketFd, const std::string message);

    void sendPerfDataToClient(int time);
};

#endif /* SERVER_H_ */
