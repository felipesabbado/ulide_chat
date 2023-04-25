#include "util.h"

int main(int argc, char *argv[]) {
    /*if(argc != 2) {
        printf("Usage: ./client <ip>\n");
        exit(EXIT_FAILURE);
    }*/

    int socketFD = createSocketConnection("127.0.0.1", PORT);

    startListeningAndPrintMessagesOnNewThread(&socketFD);
    sendMessagesToServer(socketFD);

    return 0;
}
