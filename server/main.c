#include "util.h"

int main() {
    int serverSocketFD = createSocketConnection();

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;
}
