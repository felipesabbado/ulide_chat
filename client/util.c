#include "util.h"

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

void clientInterface(int result, char **name, char **line, size_t *lineSize) {
    (*name) = NULL;
    (*line) = NULL;
    (*lineSize) = 0;

    if(result == 0)
        printf("connect was successfully\n");
    else
        printf("error %d", result);

    size_t nameSize = 0;
    printf("Please, enter your nickname \n");
    ssize_t nameCount = getline(name, &nameSize, stdin);
    (*name)[nameCount - 1] = 0;

    printf("Type your message(\\q to exit): \n");
}

void startListeningAndPrintMessagesOnNewThread(int socketFD) {
    pthread_t id;
    pthread_create(&id, NULL, listenAndPrint, (void*) socketFD);
}

void* listenAndPrint(void* socketFD) {
    char buffer[1024];

    while(1) {
        ssize_t amountReceived = recv((int) socketFD, buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }

    close((int) socketFD);
}

void receiveAndPrintIncomingMessage(int socketFD, const char *name, char **line, size_t *lineSize) {
    char buffer[1024];

    while(1) {
        ssize_t charCount = getline(line, lineSize, stdin);
        (*line)[charCount - 1] = 0;

        sprintf(buffer, "%s: %s", name, (*line));

        if(charCount > 0) {
            if(strcmp((*line), "\\q") == 0)
                break;

            ssize_t amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
        }
    }

    close(socketFD);
}