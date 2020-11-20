#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "utils.hpp"
#include <string>
#include <poll.h>

#define POLL_INTERVAL 150

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    bool done = false;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct pollfd pollFds[2];
    char buffer[4096];

    pollFds[0].fd = STDIN_FILENO;
    pollFds[0].events = POLLIN;
    pollFds[0].revents = 0;

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

    pollFds[1].fd = sockfd;
    pollFds[1].events = POLLIN;
    pollFds[1].revents = 0;

    printf("Enter message to send to server: \n");

    while (!done) {
        bzero(buffer, 4096);

        if (poll(pollFds, 2, POLL_INTERVAL) == -1){
            error("ERROR on polling");
        }

        if (pollFds[0].revents && POLLIN) {
            fgets(buffer, 4096, stdin);

            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0){ 
                error("ERROR writing to stdin");
            }
        }

        if (pollFds[1].revents && POLLIN) {
            bzero(buffer, 4096);

            n = recv(sockfd, buffer, 4095, MSG_DONTWAIT);
            if (n < 0) {
                error("ERROR reading from socket");
            }

            if (n > 0) {
                printf("Recieved: %s\n", buffer);

                if (std::string(buffer).substr(strlen(buffer) - 4, 4) == "done") {
                    done = true;
                }
            }
        }
    }
    
    close(sockfd);
    printf("Closed connection with server.\n");

    return 0;
}
