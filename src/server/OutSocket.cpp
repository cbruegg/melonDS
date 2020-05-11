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

#include "OutSocket.h"

// TODO Error handling

OutSocket::OutSocket() {
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

#ifdef _WIN32

void OutSocket::ensureAcceptedClient() {
    while (clientSock == INVALID_SOCKET) {
        clientSock = accept(this->listenSock, nullptr, nullptr);
    }
}

void OutSocket::resetAndAcceptNewClient() {
    if (clientSock != INVALID_SOCKET) {
        closesocket(clientSock);
        clientSock = INVALID_SOCKET;
    }
    ensureAcceptedClient();
}

#endif

void OutSocket::writeData(u32 *data, size_t len) {
    this->openIfRequired();
#ifdef _WIN32
    ensureAcceptedClient();
    while (send(clientSock, reinterpret_cast<const char *>(data), len * sizeof(u32) / sizeof(char), 0) ==
           SOCKET_ERROR) {
        resetAndAcceptNewClient();
    }
#else
    write(this->fd, data, len);
#endif
}

void OutSocket::writeData(int16_t *data, size_t len) {
    this->openIfRequired();
#ifdef _WIN32
    ensureAcceptedClient();
    while (send(clientSock, reinterpret_cast<const char *>(data), len * sizeof(int16_t) / sizeof(char), 0) ==
           SOCKET_ERROR) {
        resetAndAcceptNewClient();
    }
#else
    write(this->fd, data, len);
#endif
}

void OutSocket::closePipe() {
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

void OutSocket::flushPipe() {
    // TODO ?
}

void OutSocket::openIfRequired() {
#ifdef _WIN32
#else
    if (this->fd == -1) {
        this->fd = open(this->name, O_WRONLY);
    }
#endif
}
