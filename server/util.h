#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

struct AcceptedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    int acceptedSuccessfully; // boolean
};

typedef struct AcceptedSocket ACCEPTEDSOCKET;

int createTCPIPv4Socket();

struct sockaddr_in* createIPv4Address(char *ip, int port);

void* receiveAndPrintIncomingData(void *arg);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(ACCEPTEDSOCKET *pSocket);

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD);

#endif //UTIL_UTIL_H
