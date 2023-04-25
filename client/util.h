#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>

#define PORT 2000
#define MAX_MSG_LEN 1024 // MSG Limit 500
#define PRV_KEY_LENGTH 4096
#define PUB_KEY_LENGTH 1024
#define MAX_NAME_LEN 20

int createSocketConnection(char *ip, int port);

void mainRoomBanner();

void startListeningAndPrintMessagesOnNewThread(int *socketFD);

void * listenAndPrintIncomingMessages(void *arg);

void sendMessagesToServer(int socketFD);

void askNickAndSendToServer(int socketFD, char *buffer);

void createKeysEVP(char **prvkey, char **pubkey);

int encryptMessage(const char *pubkey, const char *buffer,
                   char **ciphertext, int *ciphertext_len);

int decryptMessage(const char *prvkey, const char *ciphertext,
                   char **plaintext);

char * createHash(char *buffer);

#endif //UTIL_UTIL_H
