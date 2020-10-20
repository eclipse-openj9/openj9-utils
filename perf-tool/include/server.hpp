#ifndef SERVER_H_
#define SERVER_H_

#include "serverClients.hpp"

#include <thread>
#include <queue>
#include <poll.h>

class Server
{
    /*
     * Data members
     */
protected:
public:
private:
    int serverSocketFd, activeNetworkClients = 0, portNo;
    bool headlessMode = true, KEEP_POLLING = true;
    std::queue<std::string> messageQueue;
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
    Server(int portNo = 9003, std::string commandFileName = "", std::string logFileName = "logs.txt");

    /* Starts server thread */
    void startServer();

    /* Handle the message queue, and sends to all clients */
    void handleMessagingClients();

    /* Join the server thread and closes server's socket */
    void shutDownServer(void);

private:
    /* Handles server functionality and polling*/
    void handleServer();

    /* Handles recieving commands for the agent from clients */
    void handleClientCommand(std::string command);

    /* Handles recieving data from agent */
    void handleAgentData(std::string data);

    void sendMessage(int socketFd, std::string message);

    void sendPerfDataToClient(void);
};

#endif /* SERVER_H_ */
