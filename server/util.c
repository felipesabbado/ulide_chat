#include "util.h"

clientSocket_t acceptedSockets[MAX_CLIENTS];
pthread_mutex_t acceptedSocketsMutex = PTHREAD_MUTEX_INITIALIZER;
int acceptedSocketsCount = 0;

room_t rooms[MAX_ROOMS];
pthread_mutex_t rooms_array_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t room_mutex[MAX_ROOMS];

int createSocketConnection(char *ip, int port) {
    struct sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if(strlen(ip) == 0)
        serverAddress.sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET, ip, &serverAddress.sin_addr.s_addr);

    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocketFD < 0) {
        perror("Socket creation error");
        close(serverSocketFD);
        exit(EXIT_FAILURE);
    }

    int result = bind(serverSocketFD,
                      (const struct sockaddr *) &serverAddress,
                              sizeof(serverAddress));

    if(result == 0)
        printf("Socket was bound successufuly\n");
    else {
        perror("Socket binding error");
        close(serverSocketFD);
        exit(EXIT_FAILURE);
    }

    listen(serverSocketFD, MAX_CLIENTS);

    return serverSocketFD;
}

void startAcceptingIncomingConnections(int serverSocketFD) {
    while(1) {
        clientSocket_t clientSocket = acceptIncomingConnection(serverSocketFD);
        int socketFD = clientSocket.clientSocketFD;

        // If the server is full, send a message to the client
        pthread_mutex_lock(&acceptedSocketsMutex);
        printf("LOG: Locking acceptedSocketsMutex\n");
        if (acceptedSocketsCount == MAX_CLIENTS) {
            char buffer[] = "\\Server is full. Try again later";
            printf("LOG: Server is full\n");
            sendResponseToTheClient(buffer, socketFD);
            close(socketFD);
        }

        // If the client was not accepted, send error message to the client
        if(!clientSocket.acceptedSuccessfully) {
            char errorMsg[40];
            printf("\\LOG: Error accepting the client. Code %d.\n", clientSocket.error);
            sprintf(errorMsg, "\\Error accepting the client. Code %d.", clientSocket.error);
            sendResponseToTheClient(errorMsg, socketFD);
            close(socketFD);
        }
        else {
            acceptedSockets[acceptedSocketsCount++] = clientSocket;
            printf("LOG: Accepted sockets count: %d\n", acceptedSocketsCount);
            creatingAThreadForEachNewClient(&acceptedSockets[acceptedSocketsCount-1]);
        }

        pthread_mutex_unlock(&acceptedSocketsMutex);
        printf("LOG: Unlocking acceptedSocketsMutex\n");
    }
}

clientSocket_t acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD,
                                (struct sockaddr *) &clientAddress,
                                        &clientAddressSize);

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
    printf("LOG: Creating a thread for a new client\n");
}

void *handlingClientCommands(void (*arg)) {
    char buffer[MAX_BUFFER_LEN];
    clientSocket_t *clientSocket = arg;
    int socketFD = clientSocket->clientSocketFD;

    char *server_prvkey = malloc(KEY_LEN);
    char *server_pubkey = malloc(KEY_LEN);

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, MAX_BUFFER_LEN, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;

            if (strncmp(buffer, "-----BEGIN RSA PUBLIC KEY-----", 30) == 0) {
                strcpy(clientSocket->pubkey, buffer);
                createKeysRSA(&server_prvkey, &server_pubkey);
                send(socketFD, server_pubkey, strlen(server_pubkey), 0);
            }
            else {
                char *text = decryptMessage(server_prvkey, buffer);
                printf("%s: %s\n", clientSocket->name, text);

                if (strcmp(text, "\\commands") == 0) {
                    commandList(socketFD);
                }
                else if (strcmp(text, "\\showrooms") == 0) {
                    showroomsCommand(text, socketFD);
                }
                else if (strncmp(text, "\\createroom", 11) == 0) {
                    createroomCommand(text, clientSocket, socketFD);
                }
                else if (strncmp(text, "\\enterroom", 10) == 0) {
                    enterroomCommand(text, clientSocket, socketFD);
                }
                else if (strcmp(text, "\\leaveroom") == 0) {
                    leaveroomCommand(text, clientSocket, socketFD);
                }
                else if (strncmp(text, "\\changenick", 11) == 0) {
                    changenickCommand(text, clientSocket, socketFD);
                }
                else if (strcmp(text, "\\showclients") == 0) {
                    showclientCommand(text, socketFD);
                }
                else if (strcmp(text, "\\quit") == 0) {
                    quitCommand(text, clientSocket, socketFD);
                }
                else if (strncmp(text, "\\", 1) == 0) {
                    sprintf(text, "Command not found. Type \\commands to see the available commands");
                    sendResponseToTheClient(text, socketFD);
                }
                else {
                    if (clientSocket->room_id == -1) {
                        sprintf(text, "You are not in a room. Type \\commands to see the available commands");
                        sendResponseToTheClient(text, socketFD);
                    }
                    else {
                        char msg[MAX_MSG_LEN + MAX_NAME_LEN + 2];
                        sprintf(msg, "%s: %s", clientSocket->name, text);
                        sendReceivedMessageToARoom(msg, socketFD, clientSocket->room_id);
                    }
                }
            }
        }

        if(amountReceived == 0)
            break;
    }

    free(server_prvkey);
    free(server_pubkey);
    close(socketFD);

    return NULL;
}

void commandList(int socketFD) {
    char commands[] = "At any time, type the commands below to:\n"
                      "\\showrooms - Show available rooms\n"
                      "\\createroom roomname - Create a new room\n"
                      "\\createroom roomname password - Create a private room with password\n"
                      "\\enterroom roomname - Enter in room roomname\n"
                      "\\leaveroom - Leave the current room\n"
                      "\\changenick yournick - Change your nickname to yournick\n"
                      "\\quit - Exit the Ulide chat";
    sendResponseToTheClient(commands, socketFD);
}

void showroomsCommand(char *buffer, int socketFD) {
    char *roomsList = calloc((MAX_NAME_LEN + 3) * MAX_ROOMS, sizeof(char));
    for(int i = 0; i < MAX_ROOMS; ++i) {
        if(strcmp(rooms[i].name, "") != 0
            || strcmp(rooms[i].password, "") != 0) {
            sprintf(roomsList, "%s%s | ", roomsList, rooms[i].name);
        }
    }
    if (strcmp(roomsList, "") == 0)
        sprintf(buffer, "No rooms have been created yet. Type \\createroom to create a new room");
    else
        sprintf(buffer, "%s", roomsList);

    sendResponseToTheClient(buffer, socketFD);
    free(roomsList);
}

void createroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD) {
    // Verify if the client is already in a room
    if (clientSocket->room_id > -1 ) {
        sprintf(buffer, "You are already in a room. Type \\leaveroom to leave the current room");
        sendResponseToTheClient(buffer, socketFD);
        return;
    }

    // Verify if the command is correct
    if (strlen(buffer) < 13) {
        sprintf(buffer, "Command incomplete. Type \\commands for more information");
        sendResponseToTheClient(buffer, socketFD);
        return;
    }

    pthread_mutex_lock(&rooms_array_mutex);
    printf("LOG: Locking mutex for the rooms array to create a room\n");
    for (int i = 0; i < MAX_ROOMS; i++) {
        // Verify if the room name is already in use
        if (strcmp(rooms[i].name, buffer + 12) == 0) {
            sprintf(buffer, "Room name is already in use. Type \\showrooms to see the available rooms");
            sendResponseToTheClient(buffer, socketFD);
            pthread_mutex_unlock(&rooms_array_mutex);
            printf("LOG: Unlocking mutex for the rooms array\n");
            return;
        }

        // Verify if there is an empty space for a new room
        if (strcmp(rooms[i].name, "") == 0) {
            pthread_mutex_init(&room_mutex[i], NULL);
            printf("LOG: Creating a mutex for room %d\n", i);
            pthread_mutex_lock(&room_mutex[i]);
            printf("LOG: Locking mutex for room %d to add the room in array and initialize its variables\n", i);

            // Break the buffer into two tokens: room name and password
            char *token = strtok(buffer + 12, " ");
            strcpy(rooms[i].name, token);
            token = strtok(NULL, " ");
            if (token == NULL)
                strcpy(rooms[i].password, "");
            else
                strcpy(rooms[i].password, token);

            rooms[i].id = i;
            rooms[i].n_clients = 1;
            clientSocket->room_id = rooms[i].id;

            pthread_mutex_unlock(&room_mutex[i]);
            printf("LOG: Unlocking mutex for room %s\n", rooms[i].name);
            sprintf(buffer, "Room %s was created and now you are in it", rooms[i].name);
            sendResponseToTheClient(buffer, socketFD);
            break;
        } else if (strcmp(rooms[i].name, buffer + 12) == 0
                    && strcmp(rooms[i].password, "") == 0) {
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

    pthread_mutex_unlock(&rooms_array_mutex);
    printf("LOG: Unlocking mutex for the rooms array\n");
}

void enterroomCommand(char *buffer, clientSocket_t *clientSocket, int socketFD) {
    if (clientSocket->room_id > -1) {
        sprintf(buffer, "You are already in a room. Type \\leaveroom to leave the current room");
        sendResponseToTheClient(buffer, socketFD);
        return;
    }

    char *token = strtok(buffer + 11, " ");
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (strcmp(rooms[i].name, token) == 0) {
            if (strcmp(rooms[i].password, "") != 0) {
                printf("LOG: Trying to enter in room %s\n", token);
                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("LOG: Room %s is a private room\n", rooms[i].name);
                    sprintf(buffer, "Room %s is a private room. Please try again.", rooms[i].name);
                    sendResponseToTheClient(buffer, socketFD);
                    return;
                }
                else if (strncmp(rooms[i].password, token, 64) != 0) {
                    printf("LOG: Wrong password %s\n", token);
                    printf("LOG: Room pass is %s\n", rooms[i].password);
                    sprintf(buffer, "Wrong password for room %s. Please try again.", rooms[i].name);
                    sendResponseToTheClient(buffer, socketFD);
                    return;
                }
            }
            pthread_mutex_lock(&room_mutex[i]);
            printf("LOG: Locking mutex for room %s to increase clients in it\n", rooms[i].name);
            rooms[i].n_clients++;
            pthread_mutex_unlock(&room_mutex[i]);
            printf("LOG: Unlocking mutex for room %s\n", rooms[i].name);
            clientSocket->room_id = rooms[i].id;
            printf("LOG: Client %s entered in room %s\n", clientSocket->name, rooms[i].name);
            sprintf(buffer, "You have entered the room %s", rooms[i].name);
            sendResponseToTheClient(buffer, socketFD);
            break;
        }
    }
    if (strncmp(buffer, "\\enterroom ", 11) == 0) {
        sprintf(buffer, "Room %s does not exist", token);
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
        if (clientSocket->room_id == rooms[i].id) {
            pthread_mutex_lock(&room_mutex[i]);
            printf("LOG: Locking mutex for room %s to decrease clients in it\n", rooms[i].name);
            decreaseClientsInARoom(i);
            pthread_mutex_unlock(&room_mutex[i]);
            printf("LOG: Unlocking mutex for room %s\n", rooms[i].name);

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
            char name[MAX_NAME_LEN];
            srand(time(NULL));
            sprintf(name, "Anonymous%d", rand() % 1000);
            sprintf(buffer, "\\This nickname is already in use. Your nickname is now %s."
                            " Type \\changenick to change it.", name);
            strcpy(clientSocket->name, name);
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
    char *clientList = malloc(MAX_CLIENTS * (MAX_NAME_LEN + 4));
    strcpy(clientList, "");
    for (int i = 0; i < acceptedSocketsCount; i++) {
        sprintf(buffer, "%s %s", acceptedSockets[i].name, rooms[acceptedSockets[i].room_id].name);
        strcat(buffer, " | ");
        strcat(clientList, buffer);
    }
    sendResponseToTheClient(clientList, socketFD);
    free(clientList);
}

void quitCommand(char *buffer, const clientSocket_t *clientSocket, int socketFD) {
    // If the client is in a room, leave the room and destroy it if it is empty
    int room_id = clientSocket->room_id;
    pthread_mutex_lock(&room_mutex[room_id]);
    printf("LOG: Locking mutex for room %s to decrease clients in it\n", rooms[room_id].name);
    decreaseClientsInARoom(room_id);
    pthread_mutex_unlock(&room_mutex[room_id]);
    printf("LOG: Unlocking mutex for room %s\n", rooms[room_id].name);
    sprintf(buffer, "%s exit the Ulide Chat", clientSocket->name);
    sendReceivedMessageToARoom(buffer, clientSocket->clientSocketFD ,room_id);

    // Remove the client from the acceptedSockets array
    pthread_mutex_lock(&acceptedSocketsMutex);
    printf("LOG: Locking mutex for acceptedSockets array to remove the exiting client\n");
    for (int i = 0; i < acceptedSocketsCount; i++) {
        if (acceptedSockets[i].clientSocketFD == socketFD) {
            for (int j = i; j < acceptedSocketsCount - 1; ++j) {
                acceptedSockets[j] = acceptedSockets[j + 1];
            }
            break;
        }
    }
    acceptedSocketsCount--;
    pthread_mutex_unlock(&acceptedSocketsMutex);
    printf("LOG: Unlocking mutex for acceptedSockets array\n");

    // Close the socket and exit the thread
    close(socketFD);
    printf("LOG: Closing the socket and exit the thread\n");
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
    int cipherLen;
    for (int i = 0; i < acceptedSocketsCount; i++) {
        if (acceptedSockets[i].clientSocketFD == socketFD) {
            char *encryptedMessage = encryptMessage(acceptedSockets[i].pubkey, buffer, &cipherLen);
            send(socketFD, encryptedMessage, cipherLen, 0);
            free(encryptedMessage);
            break;
        }
    }
}

void sendReceivedMessageToARoom(char *buffer, int socketFD, int room_id) {
    for(int i = 0; i < acceptedSocketsCount; i++)
        if(acceptedSockets[i].clientSocketFD != socketFD
           && (acceptedSockets[i].room_id == room_id)) {
            int cipherLen;
            char *encryptedMessage = encryptMessage(acceptedSockets[i].pubkey, buffer, &cipherLen);
            send(acceptedSockets[i].clientSocketFD, encryptedMessage, cipherLen, 0);
        }
}

void createKeysRSA(char **prvkey, char **pubkey) {
    RSA *rsa = RSA_new();
    BIGNUM *bn = BN_new();

    BN_set_word(bn, RSA_F4);

    int ret = RSA_generate_key_ex(rsa, KEY_LEN, bn, NULL);

    if (ret != 1) {
        perror("Erro ao gerar a chave RSA");
        exit(EXIT_FAILURE);
    }

    BIO *bio_prvkey_mem = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(bio_prvkey_mem, rsa, NULL, NULL, 0, NULL, NULL);
    BIO_get_mem_data(bio_prvkey_mem, prvkey);

    // Salvar a chave em uma string do tipo PEM
    BIO *bio_mem = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPublicKey(bio_mem, rsa);
    BIO_get_mem_data(bio_mem, pubkey);

    // Liberar a memória alocada
    RSA_free(rsa);
    BN_free(bn);
}

char * encryptMessage(const char *pubkey, const char *buffer,
                      int *ciphertext_len) {
    BIO *bio_mem = BIO_new(BIO_s_mem());
    BIO_write(bio_mem, pubkey, (int) strlen(pubkey));
    RSA *rsa_key = PEM_read_bio_RSAPublicKey(bio_mem, NULL, NULL, NULL);
    BIO_free(bio_mem);

    int rsa_len = RSA_size(rsa_key);
    char *ciphertext = malloc(rsa_len);
    memset(ciphertext, 0, rsa_len);

    *ciphertext_len = RSA_public_encrypt((int) strlen(buffer),
                                         (unsigned char*)buffer,
                                         (unsigned char*)ciphertext,
                                         rsa_key, RSA_PKCS1_PADDING);

    RSA_free(rsa_key);

    return ciphertext;
}

char * decryptMessage(const char *prvkey, const char *ciphertext) {
    BIO *bio_mem = BIO_new(BIO_s_mem());
    BIO_write(bio_mem, prvkey, (int) strlen(prvkey));
    RSA *rsa_key = PEM_read_bio_RSAPrivateKey(bio_mem, NULL, NULL, NULL);
    BIO_free(bio_mem);

    char *plaintext = malloc(MAX_MSG_LEN);
    memset(plaintext, 0, MAX_MSG_LEN);

    RSA_private_decrypt(RSA_size(rsa_key),
                        (unsigned char*)ciphertext,
                        (unsigned char*)plaintext,
                        rsa_key, RSA_PKCS1_PADDING);

    RSA_free(rsa_key);

    return plaintext;
}
