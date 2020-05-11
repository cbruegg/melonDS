//
// Created by mail on 08/05/2020.
//

#ifndef MELONDS_OUTSOCKET_H
#define MELONDS_OUTSOCKET_H


#include <cstdio>
#include "../types.h"

#ifdef _WIN32

#include <windows.h>
#include <cstdint>

#endif

class OutSocket {
public:
    explicit OutSocket();

    void writeData(u32 *data, size_t len);

    void writeData(int16_t *data, size_t len);

    void closePipe();

    void flushPipe();

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


#endif //MELONDS_OUTSOCKET_H
