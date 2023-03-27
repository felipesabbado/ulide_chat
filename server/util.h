#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#define PORT 2000
#define MAX_CLIENTS 50
#define MAX_ROOM_CLIENTS 10
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

typedef struct clientExample {
    int clientFD;
    char name[MAX_NAME_LEN];
    int room_id;
} client_t;

typedef struct room {
    int id;
    char name[MAX_NAME_LEN];
    int n_clients;
    client_t clients[MAX_ROOM_CLIENTS];
    char history[MAX_ROOM_CLIENTS * MAX_MSG_LEN];
} room_t;

struct clientSocket {
    int clientSocketFD;
    struct sockaddr_in address;
    char name[MAX_NAME_LEN];
    int error;
    int acceptedSuccessfully; // boolean
};

typedef struct clientSocket clientSocket_t;

int createSocketConnection();

struct sockaddr_in* createIPv4Address(char *ip, int port);

void* receiveAndPrintIncomingData(void *arg);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(clientSocket_t *clientSocket);

void sendResponseToTheClient(char* buffer, int socketFD);

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD);

// Multiples Rooms
void send_to_room(int room_id, char *msg, int sender_fd);
void send_to_client(int fd, char *msg);
void *handle_client(void *arg);
int find_empty_room();

#endif //UTIL_UTIL_H
