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
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>

#define PORT 2000
#define MAX_MSG_LEN 1024 // MSG Limit 500
#define KEY_LENGTH 4096
#define MAX_NAME_LEN 20

int createSocketConnection(char *ip, int port);

void mainRoomBanner();

void startListeningAndPrintMessagesOnNewThread(int *socketFD);

void* listenAndPrintIncomingMessages(void *arg);

void sendMessagesToServer(int socketFD);

void createKeysRSA(char **prvkey, char **pubkey);

char * encryptMessage(const char *pubkey, const char *buffer,
                      int *ciphertext_len);

char * decryptMessage(const char *prvkey, const char *ciphertext);

char * createHash(char *buffer);

#endif //UTIL_UTIL_H
