#include <stdio.h>
// #include <sys/socket.h>
#include <winsock2.h>


int main() {
    socket(AF_INET, SOCK_STREAM, 0);

    return 0;
}