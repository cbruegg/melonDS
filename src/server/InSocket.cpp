//
// Created by mail on 08/05/2020.
//

#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <cstdint>
#include <iostream>
#include <thread>

#else

#include <zconf.h>
#include <sys/socket.h>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include "InSocket.h"

#ifdef _WIN32
#define LASTERR() WSAGetLastError()
#else
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define closesocket(socket) close(socket)
typedef int SOCKET;
#define LASTERR() errno
#endif

// TODO Error handling

InSocket::InSocket() {
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        int lastError = LASTERR();
        std::cerr << "Could not create socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not create socket");
    }

    sockaddr_in saServer{};
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr("127.0.0.1");
    saServer.sin_port = htons(0);

    if (bind(listenSock, reinterpret_cast<const sockaddr *>(&saServer), sizeof(saServer)) == SOCKET_ERROR) {
        int lastError = LASTERR();
        std::cerr << "Could not bind socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not bind socket");
    }

    int saServerLen = sizeof(saServer);
#ifdef _WIN32
    int sockNameResult = getsockname(listenSock, (struct sockaddr *) &saServer, &saServerLen);
#else
    int sockNameResult = getsockname(listenSock, (struct sockaddr *) &saServer,
                                     reinterpret_cast<socklen_t *>(&saServerLen));
#endif
    if (sockNameResult == 0) {
        port = ntohs(saServer.sin_port);
    } else {
        int lastError = LASTERR();
        std::cerr << "Could not detect port, error " << lastError << std::endl;
        closesocket(listenSock);
        throw std::runtime_error("Could not detect port");
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        int lastError = LASTERR();
        std::cerr << "Could not listen on socket, error " << lastError << std::endl;
        closesocket(listenSock);
        throw std::runtime_error("Could not listen on socket");
    }

    this->listenSock = listenSock;
    this->clientSock = INVALID_SOCKET;
}

void InSocket::closePipe() {
    if (clientSock != INVALID_SOCKET) {
        closesocket(clientSock);
        clientSock = INVALID_SOCKET;
    }
    if (listenSock != INVALID_SOCKET) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
    }
}


void InSocket::ensureAcceptedClient() {
    while (clientSock == -1) {
        clientSock = accept(this->listenSock, nullptr, nullptr);
    }
}

void InSocket::resetAndAcceptNewClient() {
    if (clientSock != -1) {
        closesocket(clientSock);
        clientSock = -1;
    }
    ensureAcceptedClient();
}

std::string InSocket::readLine() {
    std::string line;

    char c = '0';
    do {
        while (recv(clientSock, &c, 1, 0) == SOCKET_ERROR) {
            resetAndAcceptNewClient();
        }
    } while (c != '\n');
    return line;
}
