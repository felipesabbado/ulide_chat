#include "util.h"

int main(int argc, char *argv[]) {
    int socketFD = createSocketConnection(argv[1]);
    int n;
    char msg[MAX_MSG_LEN];

    printf("At any time, type the commands below to:\n"
           "\\quit - Leave the current room or log out\n"
           "\\showrooms - Show available rooms\n"
           "\\createroom - Create a new room\n"
           "\\enterroom - Enter in a room\n"
           "\\changenick - Change your nickname\n"
           "\\commands - Show this list of commands\n");

    while (1) {
        fgets(msg, MAX_MSG_LEN, stdin);
        msg[strlen(msg)-1] = '\0';

        write(socketFD, msg, strlen(msg));

        if (strcmp(msg, "\\quit") == 0)
            break;

        if (strcmp(msg, "\\showrooms") == 0)
            break;

        if (strcmp(msg, "\\enterroom") == 0) {
            startListeningAndPrintMessagesOnNewThread(socketFD);
            sendMessagesToSever(socketFD);
        }
    }

    close(socketFD);

    return 0;
}
