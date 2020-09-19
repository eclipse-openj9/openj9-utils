#ifndef SERVER_H_
#define SERVER_H_

#include <string>

// Handle sending message to all clients.
void sendMessageToClients(std::string message);
void sendPerfDataToClient(void);

// Starts the server and sends a connection succeeded message.
void startServer(int portNo);

void shutDownServer();

#endif /* SERVER_H_ */
