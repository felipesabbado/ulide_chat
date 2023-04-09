#include "util.h"

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
            if (strncmp(buffer, "\\Error", 6) == 0
            || strncmp(buffer, "\\Server", 7) == 0) {
                printf("%s\n", buffer);
                close(*socketFD);
                exit(0);
            }

            buffer[amountReceived] = 0;
            printf("%s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }

    close(*socketFD);

    return NULL;
}

void sendMessagesToServer(int socketFD) {
    char buffer[MAX_MSG_LEN];
    char *name;
    size_t nameSize = 0;

    printf("Enter your nickname: ");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    name[nameCount - 1] = 0;

    sprintf(buffer, "\\changenick %s", name);

    send(socketFD, buffer, strlen(buffer), 0);

    while (1) {
        char *msg = NULL;
        size_t lineSize = 0;

        ssize_t charCount = getline(&msg, &lineSize, stdin);
        msg[charCount - 1] = 0;

        sprintf(buffer, "%s", msg);

        if (charCount > 0) {
            if (strcmp(msg, "\\quit") == 0) {
                send(socketFD, buffer, strlen(buffer), 0);
                break;
            }

            send(socketFD, buffer, strlen(buffer), 0);
        }
    }

    close(socketFD);
}
