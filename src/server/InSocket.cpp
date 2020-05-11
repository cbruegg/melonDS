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

#include <sys/stat.h>
#include <fcntl.h>
#include <zconf.h>
#include <sys/socket.h>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    const auto listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == -1) {
        int lastError = errno;
        std::cerr << "Could not create socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not create socket");
    }

    sockaddr_in saServer{};
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr("127.0.0.1");
    saServer.sin_port = htons(0);

    if (bind(listenSock, reinterpret_cast<const sockaddr *>(&saServer), sizeof(saServer)) == -1) {
        int lastError = errno;
        std::cerr << "Could not bind socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not bind socket");
    }

    int saServerLen = sizeof(saServer);
    if (getsockname(listenSock, (struct sockaddr *) &saServer, reinterpret_cast<socklen_t *>(&saServerLen)) == 0) {
        port = ntohs(saServer.sin_port);
    } else {
        int lastError = errno;
        std::cerr << "Could not detect port, error " << lastError << std::endl;
        close(listenSock);
        throw std::runtime_error("Could not detect port");
    }

    if (listen(listenSock, 1) == -1) {
        int lastError = errno;
        std::cerr << "Could not listen on socket, error " << lastError << std::endl;
        close(listenSock);
        throw std::runtime_error("Could not listen on socket");
    }

    this->listenSock = listenSock;
    this->clientSock = -1;
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
    if (clientSock != -1) {
        close(clientSock);
        clientSock = -1;
    }
    if (listenSock != -1) {
        close(listenSock);
        listenSock = -1;
    }
#endif
}


void InSocket::ensureAcceptedClient() {
    while (clientSock == -1) {
        clientSock = accept(this->listenSock, nullptr, nullptr);
    }
}

void InSocket::resetAndAcceptNewClient() {
    if (clientSock != -1) {
#ifdef _WIN32
        closesocket(clientSock);
#else
        close(clientSock);
#endif
        clientSock = -1;
    }
    ensureAcceptedClient();
}

std::string InSocket::readLine() {
    std::string line;

#ifdef _WIN32
    char c = '0';
    do {
        while (recv(clientSock, &c, 1, 0) == SOCKET_ERROR) {
            resetAndAcceptNewClient();
        }
    } while (c != '\n');
#else
    char c = '0';
    do {
        while (recv(clientSock, &c, 1, 0) == -1) {
            resetAndAcceptNewClient();
        }
    } while (c != '\n');
#endif
    return line;
}
