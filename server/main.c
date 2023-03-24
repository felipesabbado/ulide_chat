#include <unistd.h>
#include "util.h"

struct AcceptedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    int acceptedSuccessfully; // boolean
};

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD);

void receiveAndPrintIncomingData(int socketFD);

int main() {

    int serverSocketFD = createTCPIPv4Socket();
    struct sockaddr_in* serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSocketFD, serverAddress, sizeof(*serverAddress));
    if(result == 0)
        printf("Socket was bound successufuly\n");

    int listenResult = listen(serverSocketFD, 10); // if listen == 0 it was successufuly

    struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);

    receiveAndPrintIncomingData(clientSocket->acceptedSocketFD);

    close(clientSocket->acceptedSocketFD);
    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;
}

void receiveAndPrintIncomingData(int socketFD) {
    char buffer[1024];

    while(1) {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("Response was: %s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }
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
