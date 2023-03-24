#include <unistd.h>
#include <pthread.h>
#include "util.h"

void startListeningAndPrintMessagesOnNewThread(int socketFD);

void* listenAndPrint(void* socketFD);

int main() {
    int socketFD = createTCPIPv4Socket();
    struct sockaddr_in* address = createIPv4Address("127.0.0.1", 2000);

    int result = connect(socketFD, address, sizeof(*address));

    if(result == 0)
        printf("connect was successfull\n");

    char* name = NULL;
    size_t nameSize = 0;
    printf("Please, enter your nickname \n");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    name[nameCount-1] = 0;

    char* line = NULL;
    size_t lineSize = 0;
    printf("Type your message(\\q to exit): \n");

    startListeningAndPrintMessagesOnNewThread(socketFD);

    char buffer[1024];

    while(1) {
        ssize_t charCount = getline(&line, &lineSize, stdin);
        line[charCount-1] = 0;

        sprintf(buffer, "%s: %s", name, line);

        if(charCount > 0) {
            if(strcmp(line, "\\q") == 0)
                break;

            ssize_t amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
        }
    }

    close(socketFD);

    return 0;
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
