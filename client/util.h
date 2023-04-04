#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#define PORT 2000
#define MAX_MSG_LEN 1024

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

typedef struct clientSocket {
    int clientSocketFD;
    struct sockaddr_in address;
    int acceptedSuccessfully; // boolean
    int error;
} clientSocket_t;

void createSocketConnection(char *ip, clientSocket_t *clientSocket);

struct sockaddr_in *createIPv4Address(char *ip, int port);

void mainRoomBanner();

void startListeningAndPrintMessagesOnNewThread(clientSocket_t *clientSocket);

void* listenAndPrintIncomingMessages(void *arg);

void sendMessagesToServer(int socketFD);

#endif //UTIL_UTIL_H
