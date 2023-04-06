#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#define PORT 2000
#define MAX_CLIENTS 50
#define MAX_ROOMS 5
#define MAX_NAME_LEN 20
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

typedef struct clientSocket clientSocket_t;
typedef struct room room_t;

struct room {
    int id; // -1 if room not "exist"
    char name[MAX_NAME_LEN];
    int n_clients;
};

struct clientSocket {
    int clientSocketFD;
    struct sockaddr_in address;
    char name[MAX_NAME_LEN];
    int room_id; // -1 if not in a room
    int error;
    int acceptedSuccessfully; // boolean
};

int createSocketConnection();

struct sockaddr_in* createIPv4Address(char *ip, int port);

void startAcceptingIncomingConnections(int serverSocketFD);

clientSocket_t acceptIncomingConnection(int serverSocketFD);

void creatingAThreadForEachNewClient(void *clientSocket);

void *handlingClientCommands(void (*arg));

void commandList(char *buffer, int socketFD);

void showroomsCommand(char *buffer, int socketFD);

void createroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void enterroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void leaveroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void changenickCommand(char *buffer, clientSocket_t *clientSocket, int socketFD);

void showclientCommand(char *buffer, int socketFD);

void quitCommand(char *buffer, const clientSocket_t *clientSocket, int socketFD);

void decreaseClientsInARoom(int room_id);

void sendResponseToTheClient(char* buffer, int socketFD);

void sendReceivedMessageToARoom(char *buffer, int socketFD, int room_id);

#endif //UTIL_UTIL_H
