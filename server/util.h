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
#define MAX_CLIENTS 100
#define MAX_ROOMS 10
#define MAX_NAME_LEN 20
#define MAX_PASSWORD_LEN 64
#define MAX_BUFFER_LEN 1024
#define MAX_MSG_LEN 500
#define KEY_LEN 4096

typedef struct clientSocket clientSocket_t;
typedef struct room room_t;

struct room {
    int id; // -1 if room not "exist"
    char name[MAX_NAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int n_clients;
};

struct clientSocket {
    int clientSocketFD;
    struct sockaddr_in address;
    char name[MAX_NAME_LEN];
    int room_id; // -1 if not in a room
    int error;
    int acceptedSuccessfully; // boolean
    char pubkey[KEY_LEN];
};

int createSocketConnection(char *ip, int port);

void startAcceptingIncomingConnections(int serverSocketFD);

clientSocket_t acceptIncomingConnection(int serverSocketFD);

void creatingAThreadForEachNewClient(void *clientSocket);

void *handlingClientCommands(void (*arg));

void commandList(int socketFD);

void roomCommands(int socketFD, int roomID);

void showroomsCommand(char *buffer, int socketFD);

void showclientsCommand(int socketFD, int room_id);

void createroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void enterroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void leaveroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void changenickCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void quitCommand(char *buffer, const clientSocket_t *clientSocket, int socketFD);

void decreaseClientsInARoom(int room_id);

void sendResponseToTheClient(char* buffer, int socketFD);

void sendReceivedMessageToARoom(char *buffer, int socketFD, int room_id);

void createKeysRSA(char **prvkey, char **pubkey);

char * encryptMessage(const char *pubkey, const char *buffer,
                      int *ciphertext_len);

char * decryptMessage(const char *prvkey, const char *ciphertext);

#endif //UTIL_UTIL_H
