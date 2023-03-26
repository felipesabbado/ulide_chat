#include "util.h"

ACCEPTEDSOCKET* acceptIncomingConnection(int serverSocketFD);
ACCEPTEDSOCKET acceptedSockets[MAX_CLIENTS];
int acceptedSocketsCount = 0;

room_t rooms[MAX_ROOMS];
pthread_mutex_t room_mutex[MAX_ROOMS];

int createTCPIPv4Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in* createIPv4Address(char *ip, int port) {
    struct sockaddr_in * address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if(strlen(ip) == 0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET, ip, &address->sin_addr.s_addr);

    return address;
}

void startAcceptingIncomingConnections(int serverSocketFD) {
    while(1) {
        ACCEPTEDSOCKET* clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;

        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
    }
}

void receiveAndPrintIncomingDataOnSeparateThread(ACCEPTEDSOCKET *pSocket) {
    pthread_t id;
    pthread_create(&id, NULL, receiveAndPrintIncomingData, pSocket);
}

void* receiveAndPrintIncomingData(void* arg) {
    char buffer[1024];
    ACCEPTEDSOCKET pSocket = *(ACCEPTEDSOCKET*) arg;
    int socketFD = pSocket.acceptedSocketFD;

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s\n", buffer);

            sendReceivedMessageToTheOtherClients(buffer, socketFD);
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);
}

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD) {
    for(int i = 0; i < acceptedSocketsCount; ++i)
        if(acceptedSockets[i].acceptedSocketFD != socketFD)
            send(acceptedSockets[i].acceptedSocketFD, buffer, strlen(buffer), 0);
}

ACCEPTEDSOCKET* acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressSize);

    ACCEPTEDSOCKET* acceptedSocket = malloc(sizeof(ACCEPTEDSOCKET));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;

    return acceptedSocket;
}

// Multiple Rooms

void send_to_room(int room_id, char *msg, int sender_fd) {
    int i, len = strlen(msg);
    for (i = 0; i < MAX_ROOM_CLIENTS; i++) {
        if (rooms[room_id].clients[i].fd != -1 && rooms[room_id].clients[i].fd != sender_fd) {
            write(rooms[room_id].clients[i].fd, msg, len);
        }
    }
    strncat(rooms[room_id].history, msg, MAX_MSG_LEN - 1);
}

void send_to_client(int fd, char *msg) {
    write(fd, msg, strlen(msg));
}

void *handle_client(void *arg) {
    client_t client = *(client_t*)arg;
    char buffer[MAX_MSG_LEN];
    int n, room_id = client.room_id;
    char join_msg[MAX_MSG_LEN];
    snprintf(join_msg, MAX_MSG_LEN, "%s joined the room.\n", client.name);
    send_to_room(room_id, join_msg, client.fd);

    while ((n = read(client.fd, buffer, MAX_MSG_LEN)) > 0) {
        buffer[n] = '\0';
        if (strcmp(buffer, "/quit\n") == 0) {
            break;
        }
        if (strcmp(buffer, "/list\n") == 0) {
            int i, len = 0;
            char room_list[MAX_ROOMS * MAX_NAME_LEN];
            for (i = 0; i < MAX_ROOMS; i++) {
                if (rooms[i].id != -1) {
                    len += snprintf(room_list + len, MAX_ROOMS * MAX_NAME_LEN - len,
                                    "%s (%d)\n", rooms[i].name, rooms[i].n_clients);
                }
            }
            send_to_client(client.fd, room_list);
            continue;
        }
        if (buffer[0] == '/') {
            send_to_client(client.fd, "Invalid command.\n");
            continue;
        }
        char msg[MAX_MSG_LEN + MAX_NAME_LEN];
        snprintf(msg, MAX_MSG_LEN + MAX_NAME_LEN, "[%s]: %s\n", client.name, buffer);
        send_to_room(room_id, msg, client.fd);
    }

    close(client.fd);
    snprintf(buffer, MAX_MSG_LEN, "%s left the room.\n", client.name);
    send_to_room(room_id, buffer, client.fd);
    rooms[room_id].n_clients--;
    rooms[room_id].clients[client.fd].fd = -1;
    pthread_mutex_unlock(&room_mutex[room_id]);
    pthread_exit(NULL);
}

int find_empty_room() {
    int i;
    for (i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].id == -1) {
            return i;
        }
    }
    return -1;
}