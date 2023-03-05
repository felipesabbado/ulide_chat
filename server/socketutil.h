#ifndef CLIENT_SOCKETUTIL_H
#define CLIENT_SOCKETUTIL_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

struct sockaddr_in* createIpv4Address(char *ip, int port);

int createTCPIpv4Socket();

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);

void receiveAndPrintIncomingData(int socketFD);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket);

void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD);

#endif //CLIENT_SOCKETUTIL_H
