#include "util.h"

int main() {
    int serverSocketFD = createSocketConnection("", PORT);

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;
}
