//
// Created by mail on 08/05/2020.
//

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <cstdint>
#include <iostream>
#include <thread>

#else
#include <sys/stat.h>
#include <fcntl.h>
#include <zconf.h>
#endif

#include "InSocket.h"

// TODO Error handling

InSocket::InSocket() {
#ifdef _WIN32
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        int lastError = WSAGetLastError();
        std::cerr << "Could not create socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not create socket");
    }

    sockaddr_in saServer{};
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr("127.0.0.1");
    saServer.sin_port = htons(0);

    if (bind(listenSock, reinterpret_cast<const sockaddr *>(&saServer), sizeof(saServer)) == SOCKET_ERROR) {
        int lastError = WSAGetLastError();
        std::cerr << "Could not bind socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not bind socket");
    }

    int saServerLen = sizeof(saServer);
    if (getsockname(listenSock, (struct sockaddr *) &saServer, &saServerLen) == 0) {
        port = ntohs(saServer.sin_port);
    } else {
        int lastError = WSAGetLastError();
        std::cerr << "Could not detect port, error " << lastError << std::endl;
        closesocket(listenSock);
        throw std::runtime_error("Could not detect port");
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        int lastError = WSAGetLastError();
        std::cerr << "Could not listen on socket, error " << lastError << std::endl;
        closesocket(listenSock);
        throw std::runtime_error("Could not listen on socket");
    }

    this->listenSock = listenSock;
    this->clientSock = INVALID_SOCKET;
#else
    mkfifo(name, 0666);
    this->fd = -1;
#endif
}

void InSocket::closePipe() {
#ifdef _WIN32
    if (clientSock != INVALID_SOCKET) {
        closesocket(clientSock);
        clientSock = INVALID_SOCKET;
    }
    if (listenSock != INVALID_SOCKET) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
    }
#else
    close(this->fd);
#endif
}

void InSocket::openIfRequired() {
#ifdef _WIN32
#else
    if (this->fd == -1) {
        this->fd = open(this->name, O_RDONLY);
    }
#endif
}

#ifdef _WIN32

void InSocket::ensureAcceptedClient() {
    while (clientSock == INVALID_SOCKET) {
        clientSock = accept(this->listenSock, nullptr, nullptr);
    }
}

void InSocket::resetAndAcceptNewClient() {
    if (clientSock != INVALID_SOCKET) {
        closesocket(clientSock);
        clientSock = INVALID_SOCKET;
    }
    ensureAcceptedClient();
}

#endif

std::string InSocket::readLine() {
    this->openIfRequired();

    std::string line;

#ifdef _WIN32
    char c = '0';
    do {
        while (recv(clientSock, &c, 1, 0) == SOCKET_ERROR) {
            resetAndAcceptNewClient();
        }
    } while (c != '\n');
#else
    char c;
    while (read(fd, &c, 1) == 1) {
        if (c != '\n') {
            line += c;
        } else {
            return line;
        }
    }
#endif
    return line;
}
