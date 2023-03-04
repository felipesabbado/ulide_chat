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

struct sockaddr_in* createIpv4Address(char *ip, int port);

int createTCPIpv4Socket();

#endif //CLIENT_SOCKETUTIL_H
