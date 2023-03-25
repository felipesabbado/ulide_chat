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

int createTCPIPv4Socket();

struct sockaddr_in* createIPv4Address(char *ip, int port);

void clientInterface(int result, char **name, char **line, size_t *lineSize);

void startListeningAndPrintMessagesOnNewThread(int socketFD);

void* listenAndPrint(void* socketFD);

void receiveAndPrintIncomingMessage(int socketFD, const char *name, char **line, size_t *lineSize);

#endif //UTIL_UTIL_H
