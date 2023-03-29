#include "util.h"

int main() {

    int serverSocketFD = createTCPIPv4Socket();
    struct sockaddr_in* serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSocketFD,
                      serverAddress, sizeof(*serverAddress));
    if(result == 0)
        printf("Socket was bound successufuly\n");

    // if listen == 0 it was successufuly
    int listenResult = listen(serverSocketFD, 10);

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;
}
