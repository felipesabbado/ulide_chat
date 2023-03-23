#include "util.h"

int main() {
    int socketFD = createTCPIPv4Socket();
    struct sockaddr_in* address = createIPv4Address("127.0.0.1", 2000);

    int result = connect(socketFD, address, sizeof(*address));

    if(result == 0)
        printf("connect was successfull\n");

    char* line = NULL;
    size_t lineSize = 0;
    printf("Type your message(\\q to exit): \n");

    while(1) {
        ssize_t charCount = getline(&line, &lineSize, stdin);

        if(charCount > 0) {
            if(strcmp(line, "\\q\n") == 0)
                break;

            ssize_t amountWasSent = send(socketFD, line, charCount, 0);
        }
    }

    return 0;
}
