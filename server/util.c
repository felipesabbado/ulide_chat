#include "util.h"

ACCEPTEDSOCKET* acceptIncomingConnection(int serverSocketFD);
// 10 is the max number of clients
ACCEPTEDSOCKET acceptedSockets[10];
int acceptedSocketsCount = 0;

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

void startAcceptingIncomingConnections(int serverSocketFD) {
    while(1) {
        ACCEPTEDSOCKET* clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;

        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
    }
}

void receiveAndPrintIncomingDataOnSeparateThread(ACCEPTEDSOCKET *pSocket) {
    pthread_t id;
    pthread_create(&id, NULL, receiveAndPrintIncomingData, pSocket);
}

void* receiveAndPrintIncomingData(void* arg) {
    char buffer[1024];
    ACCEPTEDSOCKET pSocket = *(ACCEPTEDSOCKET*) arg;
    int socketFD = pSocket.acceptedSocketFD;

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s\n", buffer);

            sendReceivedMessageToTheOtherClients(buffer, socketFD);
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);
}

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD) {
    for(int i = 0; i < acceptedSocketsCount; ++i)
        if(acceptedSockets[i].acceptedSocketFD != socketFD)
            send(acceptedSockets[i].acceptedSocketFD, buffer, strlen(buffer), 0);
}

ACCEPTEDSOCKET* acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressSize);

    ACCEPTEDSOCKET* acceptedSocket = malloc(sizeof(ACCEPTEDSOCKET));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;

    return acceptedSocket;
}