#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

#include "psopipe.h"

int pipe(int fildes[2]) {
    int tcp1 = -1, tcp2 = -1;
    struct sockaddr_in name;
    int namelen = sizeof(name);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }

    SOCKET tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp == INVALID_SOCKET) {
        goto clean;
    }

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(tcp, (struct sockaddr*)&name, namelen) == SOCKET_ERROR) {
        goto clean;
    }

    if (listen(tcp, 5) == SOCKET_ERROR) {
        goto clean;
    }

    if (getsockname(tcp, (struct sockaddr*)&name, &namelen) == SOCKET_ERROR) {
        goto clean;
    }

    tcp1 = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp1 == INVALID_SOCKET) {
        goto clean;
    }

    if (connect(tcp1, (struct sockaddr*)&name, namelen) == SOCKET_ERROR) {
        goto clean;
    }

    tcp2 = accept(tcp, (struct sockaddr*)&name, &namelen);
    if (tcp2 == INVALID_SOCKET) {
        goto clean;
    }

    closesocket(tcp);
    fildes[0] = tcp1;
    fildes[1] = tcp2;
    return 0;

clean:
    if (tcp != INVALID_SOCKET) {
        closesocket(tcp);
    }
    if (tcp2 != INVALID_SOCKET) {
        closesocket(tcp2);
    }
    if (tcp1 != INVALID_SOCKET) {
        closesocket(tcp1);
    }
    WSACleanup();
    return -1;
}
