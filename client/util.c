#include "util.h"

char server_pubkey[PUB_KEY_LENGTH];
char client_prvkey[PRV_KEY_LENGTH];
char client_pubkey[PUB_KEY_LENGTH];

int createSocketConnection(char *ip, int port) {
    struct sockaddr_in clientAddress;

    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    clientAddress.sin_addr.s_addr = inet_addr(ip);

    int clientSocketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocketFD < 0) {
        perror("Socket creation error");
        close(clientSocketFD);
        exit(EXIT_FAILURE);
    }

    int connectionStatus = connect(clientSocketFD,
                                   (struct sockaddr *) &clientAddress,
                                   sizeof(clientAddress));


    if (connectionStatus != 0) {
        perror("Connection error");
        close(clientSocketFD);
        exit(EXIT_FAILURE);
    }

    mainRoomBanner();

    return clientSocketFD;
}

void mainRoomBanner() {
    printf("***************************************************************\n");
    printf("|  _   _  _     ___  ___   ___         ___  _  _  ___  _____  |\n"
           "| | | | || |   |_ _||   \\ | __|       / __|| || |/   \\|_   _| |\n"
           "| | |_| || |__  | | | |) || _|       | (__ | __ || - |  | |   |\n"
           "|  \\___/ |____||___||___/ |___|       \\___||_||_||_|_|  |_|   |\n");
    printf("***************************************************************\n");
    printf("******************* Type \\commands for help *******************\n");
    printf("********** Encrypting communications, please wait... **********\n");
}

void startListeningAndPrintMessagesOnNewThread(int *socketFD) {
    pthread_t id;
    pthread_create(&id, NULL, listenAndPrintIncomingMessages, socketFD);
}

void * listenAndPrintIncomingMessages(void *arg) {
    char buffer[MAX_MSG_LEN];
    int *socketFD = (int *) arg;

    while(1) {
        ssize_t amountReceived = recv(*socketFD, buffer, MAX_MSG_LEN, 0);

        if(amountReceived > 0) {
            if (strncmp(buffer, "-----BEGIN PUBLIC KEY-----", 26) == 0) {
                strcpy(server_pubkey, buffer);
            }
            else {
                char *decryptedMsg;
                decryptMessage(client_prvkey, buffer, &decryptedMsg);

                if (strncmp(decryptedMsg, "\\Error", 6) == 0
                    || strncmp(decryptedMsg, "\\Server", 7) == 0) {
                    printf("%s\n", decryptedMsg);
                    close(*socketFD);
                    //free(decryptedMsg);
                    exit(0);
                }
                else {
                    buffer[amountReceived] = 0;
                    printf("%s\n", decryptedMsg);
                    //free(decryptedMsg);
                }
            }
        }

        if(amountReceived == 0)
            break;
    }

    close(*socketFD);

    return NULL;
}

void sendMessagesToServer(int socketFD) {
    char buffer[MAX_MSG_LEN];
    char *prvkey = NULL;
    char *pubkey = NULL;

    // Create the RSA keys and send the public key to the server
    createKeysEVP(&prvkey, &pubkey);
    strcpy(client_prvkey, prvkey);
    strcpy(client_pubkey, pubkey);
    free(prvkey);
    free(pubkey);
    send(socketFD, client_pubkey, strlen(client_pubkey), 0);

    while (strlen(server_pubkey) == 0) {
        sleep(1);
    }

    // Ask for the nickname
    askNickAndSendToServer(socketFD, buffer);

    while (1) {
        char *msg = NULL;
        int ciphermsg_len;
        size_t lineSize = 0;

        ssize_t charCount = getline(&msg, &lineSize, stdin);
        msg[charCount - 1] = 0;

        sprintf(buffer, "%s", msg);
        char *ciphermsg;

        if (charCount > 0) {
            if (strcmp(msg, "\\quit") == 0) {
                encryptMessage(server_pubkey, buffer, &ciphermsg, &ciphermsg_len);
                send(socketFD, ciphermsg, ciphermsg_len, 0);
                //free(ciphermsg);
                break;
            }
            else if (strncmp(msg, "\\createroom ", 12) == 0) {
                char *token = strtok(msg + 12, " ");
                char *roomName = token;
                token = strtok(NULL, " ");
                if (token != NULL) {
                    char *password = createHash(token);
                    sprintf(buffer, "\\createroom %s %s", roomName, password);
                }
            }
            else if (strncmp(msg, "\\enterroom ", 11) == 0) {
                char *token = strtok(msg + 11, " ");
                char *roomName = token;
                token = strtok(NULL, " ");
                if (token != NULL) {
                    char *password = createHash(token);
                    sprintf(buffer, "\\enterroom %s %s", roomName, password);
                }
            }

            encryptMessage(server_pubkey, buffer, &ciphermsg, &ciphermsg_len);
            send(socketFD, ciphermsg, ciphermsg_len, 0);
            //free(ciphermsg);
        }
    }

    close(socketFD);
}

void askNickAndSendToServer(int socketFD, char *buffer) {
    char *name = malloc(MAX_NAME_LEN);
    char *ciphertext;
    int ciphertext_len;
    size_t nameSize = 0;

    while (strlen(server_pubkey) == 0) {
        usleep(500000);
    }

    printf("Enter your nickname: ");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    name[nameCount - 1] = 0;

    sprintf(buffer, "\\changenick %s", name);

    encryptMessage(server_pubkey, buffer, &ciphertext, &ciphertext_len);

    send(socketFD, ciphertext, ciphertext_len, 0);
    free(name);
    //free(ciphertext);
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

int decryptMessage(const char *prvkey, const char *ciphertext,
                   char **plaintext) {
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

char * createHash(char *buffer) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    char hash_hex_str[(EVP_MAX_MD_SIZE * 2) + 1];

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(mdctx, buffer, strlen(buffer));
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);

    // Converter o valor do hash para uma string em formato hexadecimal
    for (int i = 0; i < hash_len; i++) {
        sprintf(&hash_hex_str[i*2], "%02x", hash[i]);
    }

    char *hash_str = malloc(hash_len * 2);
    memcpy(hash_str, hash_hex_str, (hash_len * 2));
    hash_str[(hash_len * 2)] = 0;

    return hash_str;
}
