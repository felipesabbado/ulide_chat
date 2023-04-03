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

int createSocketConnection(char* ip);

struct sockaddr_in* createIPv4Address(char *ip);

void mainRoomBanner();

void startListeningAndPrintMessagesOnNewThread(int socketFD);

void* listenAndPrintIncomingMessages(void* socketFD);

void sendMessagesToServer(int socketFD);

#endif //UTIL_UTIL_H
