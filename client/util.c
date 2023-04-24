#include "util.h"

char server_pubkey[MAX_MSG_LEN];
char client_prvkey[KEY_LENGTH];
char client_pubkey[KEY_LENGTH];

void askNickAndSendToServer(int socketFD, char *buffer);

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
}

void startListeningAndPrintMessagesOnNewThread(int *socketFD) {
    pthread_t id;
    pthread_create(&id, NULL, listenAndPrintIncomingMessages, socketFD);
}

void *listenAndPrintIncomingMessages(void *arg) {
    char buffer[MAX_MSG_LEN];
    int *socketFD = (int *) arg;

    while(1) {
        ssize_t amountReceived = recv(*socketFD, buffer, MAX_MSG_LEN, 0);

        if(amountReceived > 0) {
            if (strncmp(buffer, "-----BEGIN RSA PUBLIC KEY-----", 30) == 0) {
                strcpy(server_pubkey, buffer);
            }
            else {
                char *decryptedMsg = decryptMessage(client_prvkey, buffer);

                if (strncmp(decryptedMsg, "\\Error", 6) == 0
                    || strncmp(decryptedMsg, "\\Server", 7) == 0) {
                    printf("%s\n", decryptedMsg);
                    close(*socketFD);
                    exit(0);
                }
                else {
                    buffer[amountReceived] = 0;
                    printf("%s\n", decryptedMsg);
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
    char *prvkey = malloc(KEY_LENGTH);
    char *pubkey = malloc(KEY_LENGTH);

    // Create the RSA keys and send the public key to the server
    createKeysRSA(&prvkey, &pubkey);
    strcpy(client_prvkey, prvkey);
    strcpy(client_pubkey, pubkey);
    free(prvkey);
    free(pubkey);
    send(socketFD, client_pubkey, strlen(client_pubkey), 0);

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
                ciphermsg = encryptMessage(server_pubkey, buffer, &ciphermsg_len);
                send(socketFD, ciphermsg, ciphermsg_len, 0);
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

            ciphermsg = encryptMessage(server_pubkey, buffer, &ciphermsg_len);
            send(socketFD, ciphermsg, ciphermsg_len, 0);
        }
    }

    close(socketFD);
}

void askNickAndSendToServer(int socketFD, char *buffer) {
    char *name = malloc(MAX_NAME_LEN);
    int ciphertext_len;
    size_t nameSize = 0;
    printf("Enter your nickname: ");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    name[nameCount - 1] = 0;

    sprintf(buffer, "\\changenick %s", name);

    char *ciphertext = encryptMessage(server_pubkey, buffer, &ciphertext_len);
    send(socketFD, ciphertext, ciphertext_len, 0);
    free(name);
}

void createKeysRSA(char **prvkey, char **pubkey) {
    RSA *rsa = RSA_new();
    BIGNUM *bn = BN_new();

    BN_set_word(bn, RSA_F4);

    int ret = RSA_generate_key_ex(rsa, KEY_LENGTH, bn, NULL);

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
    free(rsa);
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

    free(rsa_key);

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

    free(rsa_key);

    return plaintext;
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
