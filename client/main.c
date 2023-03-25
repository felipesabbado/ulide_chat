#include "util.h"

int main() {
    int socketFD = createTCPIPv4Socket();
    struct sockaddr_in* address = createIPv4Address("127.0.0.1", 2000);

    int result = connect(socketFD, address, sizeof(*address));

    char *name;
    char *line;
    size_t lineSize;
    clientInterface(result, &name, &line, &lineSize);

    startListeningAndPrintMessagesOnNewThread(socketFD);

    receiveAndPrintIncomingMessage(socketFD, name, &line, &lineSize);

    return 0;
}
