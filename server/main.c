#include <unistd.h>
#include <pthread.h>
#include "util.h"

struct AcceptedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    int acceptedSuccessfully; // boolean
};

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD);

void* receiveAndPrintIncomingData(void *arg);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket);

void startAcceptingIncomingConnections(int serverSocketFD) {
    while(1) {
        struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);

        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
    }
}

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket) {
    pthread_t id;
    pthread_create(&id, NULL, receiveAndPrintIncomingData, (void*)pSocket->acceptedSocketFD);
}

void* receiveAndPrintIncomingData(void* arg) {
    char buffer[1024];
    int socketFD = (int) arg;

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("Response was: %s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }

    close(socketFD);
}

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressSize);

    struct AcceptedSocket* acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;

    return acceptedSocket;
}

int main() {

    int serverSocketFD = createTCPIPv4Socket();
    struct sockaddr_in* serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSocketFD, serverAddress, sizeof(*serverAddress));
    if(result == 0)
        printf("Socket was bound successufuly\n");

    int listenResult = listen(serverSocketFD, 10); // if listen == 0 it was successufuly

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;
}
