#include "util.h"

clientSocket_t acceptedSockets[MAX_CLIENTS];
pthread_mutex_t acceptedSocketsMutex = PTHREAD_MUTEX_INITIALIZER;
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

struct sockaddr_in *createIPv4Address(char *ip, int port) {
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
        clientSocket_t clientSocket = acceptIncomingConnection(serverSocketFD);
        int socketFD = clientSocket.clientSocketFD;

        // If the server is full, send a message to the client
        pthread_mutex_lock(&acceptedSocketsMutex);
        if (acceptedSocketsCount == MAX_CLIENTS) {
            char buffer[] = "\\Server is full. Try again later";
            printf("Server is full\n");
            sendResponseToTheClient(buffer, socketFD);
            close(socketFD);
        }

        // If the client was not accepted, send error message to the client
        if(!clientSocket.acceptedSuccessfully) {
            char errorMsg[40];
            printf("\\Error accepting the client. Code: %d.\n", clientSocket.error);
            sprintf(errorMsg, "\\Error accepting the client. Code: %d.", clientSocket.error);
            sendResponseToTheClient(errorMsg, socketFD);
            close(socketFD);
        }
        else {
            acceptedSockets[acceptedSocketsCount++] = clientSocket;
            creatingAThreadForEachNewClient(&acceptedSockets[acceptedSocketsCount-1]);
        }

        pthread_mutex_unlock(&acceptedSocketsMutex);
    }
}

clientSocket_t acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressSize);

    clientSocket_t *acceptedSocket = malloc(sizeof(clientSocket_t));
    acceptedSocket->address = clientAddress;
    acceptedSocket->clientSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;
    acceptedSocket->room_id = -1;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;

    return *acceptedSocket;
}

void creatingAThreadForEachNewClient(void *clientSocket) {
    pthread_t id;
    pthread_create(&id, NULL, handlingClientCommands, clientSocket);
}

void *handlingClientCommands(void (*arg)) {
    char buffer[MAX_MSG_LEN];
    clientSocket_t *clientSocket = arg;
    int socketFD = clientSocket->clientSocketFD;

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, MAX_MSG_LEN, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s: %s\n", clientSocket->name, buffer);

            if (strcmp(buffer, "\\commands") == 0) {
                commandList(buffer, socketFD);
            }
            else if (strcmp(buffer, "\\showrooms") == 0) {
                showroomsCommand(buffer, socketFD);
            }
            else if (strncmp(buffer, "\\createroom ", 12) == 0) {
                createroomCommand(buffer, clientSocket, socketFD);
            }
            else if (strncmp(buffer, "\\enterroom ", 11) == 0) {
                enterroomCommand(buffer, clientSocket, socketFD);
            }
            else if (strcmp(buffer, "\\leaveroom") == 0) {
                leaveroomCommand(buffer, clientSocket, socketFD);
            }
            else if (strncmp(buffer, "\\changenick ", 12) == 0) {
                changenickCommand(buffer, clientSocket, socketFD);
            }
            else if (strcmp(buffer, "\\showclients") == 0) {
                showclientCommand(buffer, socketFD);
            }
            else if (strcmp(buffer, "\\quit") == 0) {
                quitCommand(buffer, clientSocket, socketFD);
            }
            else if (strncmp(buffer, "\\", 1) == 0) {
                sprintf(buffer, "Command not found. Type \\commands to see the available commands");
                sendResponseToTheClient(buffer, socketFD);
            }
            else {
                if (clientSocket->room_id == -1) {
                    sprintf(buffer, "You are not in a room. Type \\commands to see the available commands");
                    sendResponseToTheClient(buffer, socketFD);
                }
                else {
                    char msg[MAX_MSG_LEN + MAX_NAME_LEN + 2];
                    sprintf(msg, "%s: %s", clientSocket->name, buffer);
                    sendReceivedMessageToARoom(msg, socketFD, clientSocket->room_id);
                }
            }
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);
}

void commandList(char *buffer, int socketFD) {
    char commands[] = "At any time, type the commands below to:\n"
                      "\\showrooms - Show available rooms\n"
                      "\\createroom - Create a new room\n"
                      "\\enterroom [roomname] - Enter in room [roomname]\n"
                      "\\leaveroom - Leave the current room\n"
                      "\\changenick [yournick] - Change your nickname to [yournick]\n"
                      "\\quit - Exit the Ulide chat";
    sprintf(buffer, "%s", commands);
    sendResponseToTheClient(buffer, socketFD);
}

void showroomsCommand(char *buffer, int socketFD) {
    char roomsList[(MAX_NAME_LEN +3) * MAX_ROOMS] = "";
    for(int i = 0; i < MAX_ROOMS; ++i) {
        if(strcmp(rooms[i].name, "") != 0) {
            strcat(roomsList, rooms[i].name);

            // Add the room id to the list for testing purposes
            char id[6];
            sprintf(id, "id: %d", rooms[i].id);
            strcat(roomsList, id);

            strcat(roomsList, " | ");
        }
    }
    if (strcmp(roomsList, "") == 0)
        sprintf(buffer, "No rooms have been created yet. Type \\createroom to create a new room");
    else
        sprintf(buffer, "%s", roomsList);

    sendResponseToTheClient(buffer, socketFD);
}

void createroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD) {
    // Verify if the client is already in a room
    if (clientSocket->room_id > -1 ) {
        buffer = "You are already in a room. Type \\leaveroom to leave the current room";
        sendResponseToTheClient(buffer, socketFD);
    }

    // Verify if the room name is already in use
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (strcmp(rooms[i].name, "") == 0) {
            pthread_mutex_init(&room_mutex[i], NULL);
            pthread_mutex_lock(&room_mutex[i]);
            strcpy(rooms[i].name, buffer + 12);
            rooms[i].id = i;
            rooms[i].n_clients = 1;
            clientSocket->room_id = rooms[i].id;
            pthread_mutex_unlock(&room_mutex[i]);
            sprintf(buffer, "Room %s was created and now you are in it", rooms[i].name);
            sendResponseToTheClient(buffer, socketFD);
            break;
        } else if (strcmp(rooms[i].name, buffer + 12) == 0) {
            sprintf(buffer, "Room name %s is already in use. Please try again.", rooms[i].name);
            sendResponseToTheClient(buffer, socketFD);
            break;
        }
    }

    // Verify if there is no more room spaces available
    if (strncmp(buffer, "\\createroom ", 12) == 0) {
        sprintf(buffer, "No more room spaces available");
        sendResponseToTheClient(buffer, socketFD);
    }

}

void enterroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD) {
    for (int i = 0; i < MAX_ROOMS; ++i) {
        if (strcmp(rooms[i].name, buffer + 11) == 0) {
            pthread_mutex_lock(&room_mutex[i]);
            rooms[i].n_clients++;
            pthread_mutex_unlock(&room_mutex[i]);
            clientSocket->room_id = rooms[i].id;
            sprintf(buffer, "You have entered the room %s", rooms[i].name);
            sendResponseToTheClient(buffer, socketFD);
            break;
        }
    }
    if (strncmp(buffer, "\\enterroom ", 11) == 0) {
        sprintf(buffer, "Room %s does not exist", buffer + 11);
        sendResponseToTheClient(buffer, socketFD);
    }
}

void leaveroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD) {
    if (clientSocket->room_id == -1) {
        sprintf(buffer, "You are not in a room. Type \\commands to see the available commands");
        sendResponseToTheClient(buffer, socketFD);
        return;
    }

    for (int i = 0; i < MAX_ROOMS; ++i) {
        //if (strcmp(rooms[i].name, clientSocket->room_name) == 0) {
        if (clientSocket->room_id == rooms[i].id) {
            pthread_mutex_lock(&room_mutex[i]);
            decreaseClientsInARoom(i);
            pthread_mutex_unlock(&room_mutex[i]);

            clientSocket->room_id = -1;

            sprintf(buffer, "%s left the room", clientSocket->name);
            sendReceivedMessageToARoom(buffer, clientSocket->clientSocketFD, rooms[i].id);

            sprintf(buffer, "You have left the room %s", rooms[i].name);
            sendResponseToTheClient(buffer, socketFD);
            break;
        }
    }
}

void changenickCommand(char *buffer, clientSocket_t *clientSocket, int socketFD) {
    // Verify if the nickname is already in use
    for (int i = 0; i < acceptedSocketsCount; i++) {
        if (strcmp(acceptedSockets[i].name, buffer + 12) == 0) {
            sprintf(buffer, "Nickname %s is already in use. Please try again.", buffer + 12);
            sendResponseToTheClient(buffer, socketFD);
            return;
        }
    }

    // If nickname isn't in use, change it
    strcpy(clientSocket->name, buffer + 12);
    sprintf(buffer, "Your nickname is now %s", clientSocket->name);
    sendResponseToTheClient(buffer, socketFD);
}

void showclientCommand(char *buffer, int socketFD) {
    char clientList[MAX_CLIENTS * (MAX_NAME_LEN + 4)] = "";
    for (int i = 0; i < acceptedSocketsCount; ++i) {
        sprintf(buffer, "%s %s", acceptedSockets[i].name, rooms[acceptedSockets[i].room_id].name);
        strcat(buffer, " | ");
        strcat(clientList, buffer);
    }
    sendResponseToTheClient(clientList, socketFD);
}

void quitCommand(char *buffer, const clientSocket_t *clientSocket, int socketFD) {// If the client is in a room,
// leave the room and destroy it if it is empty
    int room_id = clientSocket->room_id;
    pthread_mutex_lock(&room_mutex[room_id]);
    decreaseClientsInARoom(room_id);
    pthread_mutex_unlock(&room_mutex[room_id]);
    sprintf(buffer, "%s left the room", clientSocket->name);
    sendReceivedMessageToARoom(buffer, clientSocket->clientSocketFD ,room_id);

    // Remove the client from the acceptedSockets array
    pthread_mutex_lock(&acceptedSocketsMutex);
    for (int i = 0; i < acceptedSocketsCount; ++i) {
        if (acceptedSockets[i].clientSocketFD == socketFD) {
            for (int j = i; j < acceptedSocketsCount - 1; ++j) {
                acceptedSockets[j] = acceptedSockets[j + 1];
            }
            break;
        }
    }
    acceptedSocketsCount--;
    pthread_mutex_unlock(&acceptedSocketsMutex);

    // Close the socket and exit the thread
    close(socketFD);
    pthread_exit(NULL);
}

void decreaseClientsInARoom(int room_id) {
    rooms[room_id].n_clients--;
    // Destroy the room if there are no clients in it
    if (rooms[room_id].n_clients == 0) {
        strcpy(rooms[room_id].name, "");
        rooms[room_id].id = -1;
    }
}

void sendResponseToTheClient(char *buffer, int socketFD) {
    for(int i = 0; i < acceptedSocketsCount; ++i)
        if(acceptedSockets[i].clientSocketFD == socketFD)
            send(acceptedSockets[i].clientSocketFD, buffer, strlen(buffer), 0);
}

void sendReceivedMessageToARoom(char *buffer, int socketFD, int room_id) {
    for(int i = 0; i < acceptedSocketsCount; i++)
        if(acceptedSockets[i].clientSocketFD != socketFD
           && (acceptedSockets[i].room_id == room_id))
            send(acceptedSockets[i].clientSocketFD, buffer, strlen(buffer), 0);
}
