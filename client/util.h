#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 2000
#define MAX_MSG_LEN 1024

int createSocketConnection(char *ip, int port);

void mainRoomBanner();

void startListeningAndPrintMessagesOnNewThread(int *socketFD);

void* listenAndPrintIncomingMessages(void *arg);

void sendMessagesToServer(int socketFD);

#endif //UTIL_UTIL_H
