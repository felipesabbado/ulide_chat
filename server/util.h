#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>

int createTCPIPv4Socket();
struct sockaddr_in* createIPv4Address(char *ip, int port);

#endif //UTIL_UTIL_H
