#include <unistd.h>
#include "util.h"

int main() {

    int serverSocketFD = createTCPIPv4Socket();
    struct sockaddr_in* serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSocketFD, serverAddress, sizeof(*serverAddress));
    if(result == 0)
        printf("Socket was bound successufuly\n");

    int listenResult = listen(serverSocketFD, 10); // if listen == 0 it was successufuly

    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressSize);

    char buffer[1024];

    while(1) {
        ssize_t amountReceived = recv(clientSocketFD, buffer, 1024, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("Response was: %s\n", buffer);
        }

        if(amountReceived == 0)
            break;
    }

    close(clientSocketFD);
    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;
}
