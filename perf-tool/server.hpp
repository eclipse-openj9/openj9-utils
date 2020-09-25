#ifndef SERVER_H_
#define SERVER_H_

#include <string>

// Handle sending message to all clients.
void sendMessageToClients(std::string message);
void sendPerfDataToClient(void);

// Starts the server thread.
void startServer(int portNo);

// Handles server functionality
void handleServer(int portNo);

// Join the server thread and closes server
void shutDownServer();

#endif /* SERVER_H_ */
