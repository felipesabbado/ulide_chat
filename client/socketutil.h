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

void startListeningAndPrintMessagesOnNewThread(int fd);

void listenAndPrint(int socketFD);

void readConsoleEntriesAndSendToServer(int socketFD);

#endif //CLIENT_SOCKETUTIL_H
