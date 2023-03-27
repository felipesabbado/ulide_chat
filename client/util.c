#include "util.h"

int createSocketConnection(char* ip) {
    int socketFD = createTCPIPv4Socket();
    //struct sockaddr_in* address = createIPv4Address(ip);
    struct sockaddr_in* address = createIPv4Address("127.0.0.1");
    int result = connect(socketFD, address, sizeof(*address));

    if(result == 0){
        mainRoomBanner();
    }
    else {
        printf("Error %d\n", result);
        exit(1);
    }

    return socketFD;
}

int createTCPIPv4Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in* createIPv4Address(char *ip) {
    struct sockaddr_in * address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(PORT);

    if(strlen(ip) == 0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
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
}

char* clientName() {
    char* name;

    size_t nameSize = 0;
    printf("Please, enter your nickname \n");
    ssize_t nameCount = getline(&name,
                                &nameSize, stdin);
    name[nameCount - 1] = 0;

    return name;
}

void startListeningAndPrintMessagesOnNewThread(int socketFD) {
    pthread_t id;
    pthread_create(&id, NULL,
                   listenAndPrintIncomingMessages, (void *) socketFD);
}

void* listenAndPrintIncomingMessages(void* socketFD) {
    char buffer[MAX_MSG_LEN];

    while(1) {
        ssize_t amountReceived = recv((int) socketFD,
                                      buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }

    close((int) socketFD);
}

void sendMessagesToSever(int socketFD) {
    char buffer[MAX_MSG_LEN];
    char *line = NULL;
    size_t lineSize = 0;

    char *name = clientName();

    printf("Type your message(\\quit to exit): \n");
    while(1) {
        ssize_t charCount = getline(&line,
                                    &lineSize, stdin);
        line[charCount - 1] = 0;

        sprintf(buffer, "%s: %s", name, line);

        if(charCount > 0) {
            if(strcmp(line, "\\quit") == 0)
                break;

            send(socketFD, buffer,
                 strlen(buffer), 0);
        }
    }
}