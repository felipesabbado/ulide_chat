#include "util.h"

int main(int argc, char *argv[]) {
    if (argv[1] == NULL) {
        printf("Usage: ./client <ip>\n");
        printf("No IP address provided. You are connected in localhost.\n");
        argv[1] = "127.0.0.1";
    }

    /*if(argc != 2) {
        printf("Usage: ./client <ip>\n");
        exit(EXIT_FAILURE);
    }*/

    int socketFD = createSocketConnection(argv[1], PORT);

    startListeningAndPrintMessagesOnNewThread(&socketFD);
    sendMessagesToServer(socketFD);

    return 0;
}
