#include "util.h"

void createSocketConnection(char *ip, clientSocket_t *clientSocket) {
    int clientSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in *clientAddress = createIPv4Address(ip, PORT);
    socklen_t clientAddressSize = sizeof(struct sockaddr_in);

    clientSocket->clientSocketFD = clientSocketFD;
    clientSocket->address = *clientAddress;
    clientSocket->acceptedSuccessfully = clientSocketFD > 0;

    int result = connect(clientSocket->clientSocketFD,
                         (struct sockaddr *) &clientSocket->address,
                                 clientAddressSize);

    if (!clientSocket->acceptedSuccessfully) {
        clientSocket->error = clientSocketFD;
        printf("Client socket error %d\n", clientSocket->error);
        exit(clientSocket->error);
    } else if(result != 0) {
        printf("Connection error %d\n", result);
        exit(result);
    }
    else
        mainRoomBanner();
}

struct sockaddr_in *createIPv4Address(char *ip, int port) {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if(ip == NULL) {
        ip = "127.0.0.1";
        address->sin_addr.s_addr = INADDR_ANY;
    }

    inet_pton(AF_INET, ip, &address->sin_addr.s_addr);

    return address;
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

void startListeningAndPrintMessagesOnNewThread(clientSocket_t *clientSocket) {
    pthread_t id;
    pthread_create(&id, NULL, listenAndPrintIncomingMessages, clientSocket);
}

void *listenAndPrintIncomingMessages(void *arg) {
    char buffer[MAX_MSG_LEN];
    clientSocket_t clientSocket = *(clientSocket_t*) arg;
    int socketFD = clientSocket.clientSocketFD;

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, MAX_MSG_LEN, 0);

        if(amountReceived > 0) {
            if (strncmp(buffer, "\\Error", 6) == 0
            || strncmp(buffer, "\\Server", 7) == 0) {
                printf("%s\n", buffer);
                close(socketFD);
                exit(0);
            }

            buffer[amountReceived] = 0;
            printf("%s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);
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
