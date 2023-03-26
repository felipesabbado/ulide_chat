#include "util.h"

int main(int argc, char *argv[]) {
    int socketFD = createSocketConnection(argv[1]);

    char *name = clientName();

    startListeningAndPrintMessagesOnNewThread(socketFD);

    receiveAndPrintIncomingMessage(socketFD, name);

    return 0;
}
