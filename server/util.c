#include "util.h"

clientSocket_t *acceptIncomingConnection(int serverSocketFD);
clientSocket_t acceptedSockets[MAX_CLIENTS];
int acceptedSocketsCount = 0;

room_t rooms[MAX_ROOMS];
pthread_mutex_t room_mutex[MAX_ROOMS];

int createSocketConnection() {
    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in *serverAddress = createIPv4Address("",PORT);

    int result = bind(serverSocketFD,serverAddress,sizeof(*serverAddress));
    if(result == 0)
        printf("Socket was bound successufuly\n");
    else
        exit(1);

    int listenResult = listen(serverSocketFD, MAX_CLIENTS);

    return serverSocketFD;
}

struct sockaddr_in* createIPv4Address(char *ip, int port) {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
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
        clientSocket_t *clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;

        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
    }
}

clientSocket_t *acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressSize);

    clientSocket_t *acceptedSocket = malloc(sizeof(clientSocket_t));
    acceptedSocket->address = clientAddress;
    acceptedSocket->clientSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;

    return acceptedSocket;
}

void receiveAndPrintIncomingDataOnSeparateThread(clientSocket_t *clientSocket) {
    pthread_t id;
    pthread_create(&id, NULL, receiveAndPrintIncomingData, clientSocket);
}

void* receiveAndPrintIncomingData(void *arg) {
    char buffer[MAX_MSG_LEN];
    clientSocket_t clientSocket = *(clientSocket_t*) arg;
    int socketFD = clientSocket.clientSocketFD;
    strcpy(rooms[0].room_name, "main");
    strcpy(rooms[1].room_name, "room1");

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, MAX_MSG_LEN, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s: %s\n", clientSocket.name, buffer);

            if (strcmp(buffer, "\\showrooms") == 0) {
                char roomsList[MAX_NAME_LEN * MAX_ROOMS] = "";
                for(int i = 0; i < MAX_ROOMS; ++i) {
                    if(strcmp(rooms[i].room_name, "") != 0) {
                        strcat(roomsList, rooms[i].room_name);
                        strcat(roomsList, " | ");
                    }
                }
                sprintf(buffer, "%s", roomsList);

                sendResponseToTheClient(buffer, socketFD);
            }
            else if (strncmp(buffer, "\\changenick ", 12) == 0) {
                strcpy(clientSocket.name, buffer + 12);
                sprintf(buffer, "Your nickname is now %s", clientSocket.name);
                sendResponseToTheClient(buffer, socketFD);
            }
            else if (strcmp(buffer, "\\commands") == 0) {
                commandList(buffer, socketFD);
            }
            else {
                char msg[MAX_MSG_LEN];
                strcpy(msg, buffer);
                sprintf(buffer, "%s: %s", clientSocket.name, msg);
                sendReceivedMessageToTheOtherClients(buffer, socketFD);
            }
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);
}

void sendResponseToTheClient(char *buffer, int socketFD) {
    for(int i = 0; i < acceptedSocketsCount; ++i)
        if(acceptedSockets[i].clientSocketFD == socketFD)
            send(acceptedSockets[i].clientSocketFD, buffer, strlen(buffer), 0);
}

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD) {
    for(int i = 0; i < acceptedSocketsCount; ++i)
        if(acceptedSockets[i].clientSocketFD != socketFD)
            send(acceptedSockets[i].clientSocketFD, buffer, strlen(buffer), 0);
}

void commandList(char *buffer, int socketFD) {
    char commands[] = "At any time, type the commands below to:\n"
           "\\quit - Leave the current room or log out\n"
           "\\showrooms - Show available rooms\n"
           "\\createroom - Create a new room\n"
           "\\enterroom - Enter in a room\n"
           "\\changenick [yournick] - Change your nickname\n"
           "\\commands - Show this list of commands";
    sprintf(buffer, "%s", commands);
    sendResponseToTheClient(buffer, socketFD);
}

// Multiple Rooms

/*
void send_to_room(int room_id, char *msg, int sender_fd) {
    int i, len = strlen(msg);
    for (i = 0; i < MAX_ROOM_CLIENTS; i++) {
        if (rooms[room_id].clients[i].clientFD != -1 && rooms[room_id].clients[i].clientFD != sender_fd) {
            write(rooms[room_id].clients[i].clientFD, msg, len);
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
    send_to_room(room_id, join_msg, client.clientFD);

    while ((n = read(client.clientFD, buffer, MAX_MSG_LEN)) > 0) {
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
            send_to_client(client.clientFD, room_list);
            continue;
        }
        if (buffer[0] == '/') {
            send_to_client(client.clientFD, "Invalid command.\n");
            continue;
        }
        char msg[MAX_MSG_LEN + MAX_NAME_LEN];
        snprintf(msg, MAX_MSG_LEN + MAX_NAME_LEN, "[%s]: %s\n", client.name, buffer);
        send_to_room(room_id, msg, client.clientFD);
    }

    close(client.clientFD);
    snprintf(buffer, MAX_MSG_LEN, "%s left the room.\n", client.name);
    send_to_room(room_id, buffer, client.clientFD);
    rooms[room_id].n_clients--;
    rooms[room_id].clients[client.clientFD].clientFD = -1;
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
}*/
