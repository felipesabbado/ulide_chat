#include "socketutil.h"

int main() {
    int socketFD = createTCPIpv4Socket(); // File Descriptor

    // Constructing address for the server
    struct sockaddr_in *address = createIpv4Address("127.0.0.1", 2000);


    int result = connect(socketFD, address, sizeof(*address));

    if (result == 0)
        printf("Connection was successful.\n");

    char *line = NULL;
    size_t lineSize = 0;
    printf("Type your message...(type exit for end)\n");

    while (true) {
        ssize_t charCount = getline(&line, &lineSize, stdin);

        if (charCount > 0) {
            if (strcmp(line, "exit\n") == 0)
                break;

            ssize_t amountWasSent = send(socketFD, line, charCount, 0);
        }

    }

    close(socketFD);

    return 0;
}
