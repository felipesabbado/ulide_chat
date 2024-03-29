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

    // Listening for incoming connections
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
    printf("LOG: Creating thread #%lu for a new client\n", id);
}

void *handlingClientCommands(void (*arg)) {
    char buffer[MAX_BUFFER_LEN];
    clientSocket_t *clientSocket = arg;
    int socketFD = clientSocket->clientSocketFD;

    char *server_prvkey = malloc(PRV_KEY_LENGTH);
    char *server_pubkey = malloc(PUB_KEY_LENGTH);

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, MAX_BUFFER_LEN, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;

            if (strncmp(buffer, "-----BEGIN PUBLIC KEY-----", 26) == 0) {
                strcpy(clientSocket->pubkey, buffer);
                createKeysEVP(&server_prvkey, &server_pubkey);
                send(socketFD, server_pubkey, strlen(server_pubkey), 0);
            }
            else {
                char *text;
                decryptMessage(server_prvkey, buffer, &text);
                printf("%s: %s\n", clientSocket->name, text);

                if (strcmp(text, "\\commands") == 0)
                    commandList(socketFD);
                else if (strcmp(text, "\\roomcommands") == 0)
                    roomCommands(socketFD, clientSocket->room_id);
                else if (strcmp(text, "\\showrooms") == 0)
                    showroomsCommand(text, socketFD);
                else if (strcmp(text, "\\showclients") == 0)
                    showclientsCommand(socketFD, clientSocket->room_id);
                else if (strncmp(text, "\\createroom", 11) == 0)
                    createroomCommand(text, clientSocket, socketFD);
                else if (strncmp(text, "\\enterroom", 10) == 0)
                    enterroomCommand(text, clientSocket, socketFD);
                else if (strcmp(text, "\\leaveroom") == 0)
                    leaveroomCommand(text, clientSocket, socketFD);
                else if (strncmp(text, "\\changenick", 11) == 0)
                    changenickCommand(text, clientSocket, socketFD);
                else if (strcmp(text, "\\quit") == 0)
                    quitCommand(text, clientSocket, socketFD);
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
                        char msg[MAX_BUFFER_LEN + MAX_NAME_LEN + 2];
                        sprintf(msg, "%s: %s", clientSocket->name, text);
                        sendReceivedMessageToARoom(msg, socketFD, clientSocket->room_id);
                    }
                }

                //free(text);
            }
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);

    return NULL;
}

void commandList(int socketFD) {
    char commands[] = "At any time, type the commands below to:\n"
                      "\\showrooms - Show available rooms and number of user in it\n"
                      "\\showclients - Show all clients in Ulide Chat\n"
                      "\\createroom roomname - Create a new room\n"
                      "\\createroom roomname password - Create a private room with password\n"
                      "\\enterroom roomname - Enter in room roomname\n"
                      "\\changenick yournick - Change your nickname to yournick\n"
                      "\\quit - Exit the Ulide chat";
    sendResponseToTheClient(commands, socketFD);
}

void roomCommands(int socketFD, int roomID) {
    if (roomID == -1) {
        char commands[] = "You are not in a room. Type \\commands to see the available commands";
        sendResponseToTheClient(commands, socketFD);
        return;
    }
    char commands[] = "In a room, type the commands below to:\n"
                      "\\showclients - Show the clients in the current room\n"
                      "\\leaveroom - Leave the current room";
    sendResponseToTheClient(commands, socketFD);
}

void showroomsCommand(char *buffer, int socketFD) {
    char *roomsList = calloc((MAX_NAME_LEN + 3) * MAX_ROOMS, sizeof(char));
    for(int i = 0; i < MAX_ROOMS; ++i) {
        if(strcmp(rooms[i].name, "") != 0
            || strcmp(rooms[i].password, "") != 0) {
            sprintf(roomsList, "%s%s(%d) | ", roomsList, rooms[i].name, rooms[i].n_clients);
        }
    }
    if (strcmp(roomsList, "") == 0)
        sprintf(buffer, "No rooms have been created yet. Type \\createroom to create a new room");
    else
        sprintf(buffer, "%s", roomsList);

    sendResponseToTheClient(buffer, socketFD);
    free(roomsList);
}

void showclientsCommand(int socketFD, int roomID) {
    char *clientList = malloc(MAX_CLIENTS * (MAX_NAME_LEN + 3));
    strcpy(clientList, "");

    if (roomID == -1) {
        for (int i = 0; i < acceptedSocketsCount; i++)
            sprintf(clientList, "%s%s | ",clientList, acceptedSockets[i].name);
    }
    else {
        for (int i = 0; i < acceptedSocketsCount; i++) {
            if (acceptedSockets[i].room_id == roomID)
                sprintf(clientList, "%s%s | ",clientList, acceptedSockets[i].name);
        }
    }

    sendResponseToTheClient(clientList, socketFD);
    free(clientList);
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
            sprintf(buffer, "Room %s was created, type \\roomcommands for help", rooms[i].name);
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
            sprintf(buffer, "You have entered the room %s, type \\roomcommands for help", rooms[i].name);
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
            char *encryptedMessage;
            encryptMessage(acceptedSockets[i].pubkey, buffer, &encryptedMessage, &cipherLen);
            send(socketFD, encryptedMessage, cipherLen, 0);
            //free(encryptedMessage);
            break;
        }
    }
}

void sendReceivedMessageToARoom(char *buffer, int socketFD, int room_id) {
    for(int i = 0; i < acceptedSocketsCount; i++)
        if(acceptedSockets[i].clientSocketFD != socketFD
           && (acceptedSockets[i].room_id == room_id)) {
            int cipherLen;
            char *encryptedMessage;
            encryptMessage(acceptedSockets[i].pubkey, buffer, &encryptedMessage, &cipherLen);
            send(acceptedSockets[i].clientSocketFD, encryptedMessage, cipherLen, 0);
            //free(encryptedMessage);
        }
}

void createKeysEVP(char **prvkey, char **pubkey) {
    EVP_PKEY_CTX *ctx;
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 4096);
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_keygen(ctx, &pkey);
    EVP_PKEY_CTX_free(ctx);

    BIO *private_key_bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(private_key_bio, pkey, NULL, NULL, 0, NULL, NULL);
    size_t private_key_len = BIO_pending(private_key_bio);
    (*prvkey) = malloc(private_key_len + 1);
    BIO_read(private_key_bio, (*prvkey), (int)private_key_len);
    (*prvkey)[private_key_len] = '\0';
    BIO_free(private_key_bio);

    BIO *public_key_bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(public_key_bio, pkey);
    size_t public_key_len = BIO_pending(public_key_bio);
    (*pubkey) = malloc(public_key_len + 1);
    BIO_read(public_key_bio, (*pubkey), (int)public_key_len);
    (*pubkey)[public_key_len] = '\0';
    BIO_free(public_key_bio);

    EVP_PKEY_free(pkey);
}

int encryptMessage(const char *pubkey, const char *buffer,
                   char **ciphertext, int *ciphertext_len) {
    BIO *bio_mem = BIO_new(BIO_s_mem());
    BIO_puts(bio_mem, pubkey);
    EVP_PKEY *evp_key = PEM_read_bio_PUBKEY(bio_mem, NULL, NULL, NULL);
    BIO_free(bio_mem);

    if (!evp_key) {
        fprintf(stderr, "Erro ao carregar a chave pública.\n");
        return -1;
    }

    EVP_PKEY_CTX *ctx;
    ctx = EVP_PKEY_CTX_new(evp_key, NULL);
    EVP_PKEY_encrypt_init(ctx);

    size_t block_size = EVP_PKEY_size(evp_key);

    *ciphertext = (char *)malloc(block_size);
    if (!*ciphertext) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evp_key);
        return -1;
    }

    size_t len;
    if (EVP_PKEY_encrypt(ctx, (unsigned char *)*ciphertext,
                         &len, (unsigned char *)buffer,
                         strlen(buffer)) != 1) {
        fprintf(stderr, "Erro ao criptografar a mensagem.\n");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evp_key);
        free(*ciphertext);
        return -1;
    }

    *ciphertext_len = (int)len;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(evp_key);

    return 0;
}

int decryptMessage(const char *prvkey, const char *ciphertext, char **plaintext) {

    BIO *bio_mem = BIO_new(BIO_s_mem());
    BIO_puts(bio_mem, prvkey);
    EVP_PKEY *evp_key = PEM_read_bio_PrivateKey(bio_mem, NULL, NULL, NULL);
    BIO_free(bio_mem);

    if (!evp_key) {
        fprintf(stderr, "Erro ao carregar a chave privada.\n");
        return -1;
    }

    EVP_PKEY_CTX *ctx;
    ctx = EVP_PKEY_CTX_new(evp_key, NULL);
    EVP_PKEY_decrypt_init(ctx);

    *plaintext = (char *)malloc(512);
    if (!*plaintext) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evp_key);
        return -1;
    }

    size_t len;
    if (EVP_PKEY_decrypt(ctx, (unsigned char *)*plaintext, &len,
                         (unsigned char *)ciphertext, 512) != 1) {
        fprintf(stderr, "Erro ao decifrar a mensagem.\n");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evp_key);
        free(*plaintext);
        return -1;
    }

    (*plaintext)[len] = '\0';

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(evp_key);

    return 0;
}
