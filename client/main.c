#include "util.h"

int main(int argc, char *argv[]) {
    int socketFD = createSocketConnection("127.0.0.1");
    //int socketFD = createSocketConnection(argv[1]);

    startListeningAndPrintMessagesOnNewThread(socketFD);
    sendMessagesToServer(socketFD);

    return 0;
}
