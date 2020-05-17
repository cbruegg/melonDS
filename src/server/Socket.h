//
// Created by mail on 08/05/2020.
//

#ifndef MELONDS_SOCKET_H
#define MELONDS_SOCKET_H


#include <cstdio>
#include "../types.h"

#ifdef _WIN32

#include <windows.h>
#include <cstdint>

#endif

class Socket {
public:
    explicit Socket();

    void writeSizeInBytes(u32 *data, int n);

    void writeSizeInBytes(int16_t *data, int n);

    void writeData(u32 *data, int n);

    void writeData(int16_t *data, int n);

    void closePipe();

    void flushPipe();

    std::string readLine();

    int port;

private:
#ifdef _WIN32
    SOCKET listenSock;
    SOCKET clientSock;
#else
    int listenSock;
    int clientSock;
#endif

    void ensureAcceptedClient();

    void resetAndAcceptNewClient();

};


#endif //MELONDS_SOCKET_H
