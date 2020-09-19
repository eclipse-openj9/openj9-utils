
#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>
#include <poll.h>

class Client
{
    /*
     * Data members
     */
protected:
public:
private:
    int socketFd, portno;
    std::string hostname;
    bool interactive_mode, keepPolling = true;
    struct pollfd pollFds[2];

    /*
     * Function members
     */
protected:
public:
    Client(const int _portno, const std::string _hostname = "localhost", bool _interactive_mode = false);

    void startClient(void);
    void closeClient(void);

private:
    void openServerConnection(void);
    void handlePolling(void);
    void sendMessage(const char message[]);
    void receiveMessage(char buffer[]);
};

#endif /* CLIENT_H_ */
