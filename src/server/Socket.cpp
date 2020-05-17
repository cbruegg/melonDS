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
#include <thread>

#else

#include <fcntl.h>
#include <zconf.h>
#include <sys/socket.h>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include "Socket.h"

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

Socket::Socket() {
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        int lastError = LASTERR();
        std::cerr << "Could not create socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not create socket");
    }

    int yes = 1;
    int result = setsockopt(listenSock,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *) &yes,
                            sizeof(int));    // 1 - on, 0 - off
    if (result < 0) {
        int lastError = LASTERR();
        std::cerr << "Could not set NODELAY on socket, error " << lastError << std::endl;
        throw std::runtime_error("Could not set NODELAY on socket");
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

void Socket::ensureAcceptedClient() {
    while (clientSock == -1) {
        clientSock = accept(this->listenSock, nullptr, nullptr);
    }
}

void Socket::resetAndAcceptNewClient() {
    if (clientSock != -1) {
        closesocket(clientSock);
        clientSock = -1;
    }
    ensureAcceptedClient();
}

int sendWrapper(SOCKET socket, void *dataToSend, size_t oneElemSize, int n) {
    char *data = (char *) dataToSend;
    auto remaining = oneElemSize * n;
    do {
        int rc = send(socket, data, remaining, 0);
        if (rc == SOCKET_ERROR) {
            return SOCKET_ERROR;
        } else {
            data += rc;
            remaining -= rc;
        }
    } while (remaining > 0);
    return 0;
}

void Socket::writeSizeInBytes(u32 *data, int n) {
    ensureAcceptedClient();
    int64_t sizeInBytes = sizeof(u32) * n;
    while (sendWrapper(clientSock, &sizeInBytes, sizeof(u32), 1) == SOCKET_ERROR) {
        resetAndAcceptNewClient();
    }
}

void Socket::writeSizeInBytes(int16_t *data, int n) {
    ensureAcceptedClient();
    int64_t sizeInBytes = sizeof(int16_t) * n;
    while (sendWrapper(clientSock, &sizeInBytes, sizeof(u32), 1) == SOCKET_ERROR) {
        resetAndAcceptNewClient();
    }
}

void Socket::writeData(u32 *data, int n) {
    ensureAcceptedClient();
    while (sendWrapper(clientSock, data, sizeof(u32), n) == SOCKET_ERROR) {
        resetAndAcceptNewClient();
    }
}

void Socket::writeData(int16_t *data, int n) {
    ensureAcceptedClient();
    while (sendWrapper(clientSock, data, sizeof(int16_t), n) == SOCKET_ERROR) {
        resetAndAcceptNewClient();
    }
}

int receive_int(int *num, SOCKET fd) {
    int32_t ret;
    char *data = (char *) &ret;
    int left = sizeof(ret);
    int rc;
    do {
        rc = recv(fd, data, left, 0);
        if (rc <= 0) { /* instead of ret */
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                // use select() or epoll() to wait for the socket to be readable again
            } else if (errno != EINTR) {
                return -1;
            }
        } else {
            data += rc;
            left -= rc;
        }
    } while (left > 0);
    *num = ntohl(ret);
    return 0;
}

void Socket::closePipe() {
    if (clientSock != INVALID_SOCKET) {
        closesocket(clientSock);
        clientSock = INVALID_SOCKET;
    }
    if (listenSock != INVALID_SOCKET) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
    }
}

void Socket::flushPipe() {
    // TODO ?
}

std::string Socket::readLine() {
    std::string line;

    char c;
    do {
        while (recv(clientSock, &c, 1, 0) == SOCKET_ERROR) {
            resetAndAcceptNewClient();
        }
    } while (c != '\n');
    return line;
}