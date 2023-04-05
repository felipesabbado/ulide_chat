#include "util.h"

int main(int argc, char *argv[]) {
    clientSocket_t clientSocket;
    //createSocketConnection("127.0.0.1", &clientSocket);
    createSocketConnection(argv[1], &clientSocket);
    int socketFD = clientSocket.clientSocketFD;

    startListeningAndPrintMessagesOnNewThread(&clientSocket);
    sendMessagesToServer(socketFD);

    return 0;
}
