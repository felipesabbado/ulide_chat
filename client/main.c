#include "util.h"

void commandList();

int main(int argc, char *argv[]) {
    int socketFD = createSocketConnection("127.0.0.1");
    //int socketFD = createSocketConnection(argv[1]);

    commandList();

    while (1) {
        char msg[MAX_MSG_LEN];
        char *line = NULL;
        size_t lineSize = 0;

        ssize_t charCount = getline(&line, &lineSize, stdin);
        line[charCount - 1] = 0;

        sprintf(msg, "%s", line);

        send(socketFD, msg, strlen(msg), 0);

        if (strcmp(msg, "\\quit") == 0)
            break;

        if (strcmp(msg, "\\enterroom") == 0) {
            startListeningAndPrintMessagesOnNewThread(socketFD);
            sendMessagesToARoom(socketFD);
        }
    }

    close(socketFD);

    return 0;
}

void commandList() {
    printf("At any time, type the commands below to:\n"
           "\\quit - Leave the current room or log out\n"
           "\\showrooms - Show available rooms\n"
           "\\createroom - Create a new room\n"
           "\\enterroom - Enter in a room\n"
           "\\changenick - Change your nickname\n"
           "\\commands - Show this list of commands\n");
}
