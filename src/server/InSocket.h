//
// Created by mail on 08/05/2020.
//

#ifndef MELONDS_INSOCKET_H
#define MELONDS_INSOCKET_H


#include <cstdio>
#include <string>
#include "../types.h"

#ifdef _WIN32
#include <windows.h>
#include <cstdint>

#endif

class InSocket {
public:
    explicit InSocket();

    std::string readLine();

    void closePipe();

#ifdef _WIN32
    int port;
#endif


private:
#ifdef _WIN32
    SOCKET listenSock;
    SOCKET clientSock;

    void ensureAcceptedClient();
    void resetAndAcceptNewClient();
#else
    const int fd;
#endif
    const char *name;

    void openIfRequired();
};


#endif //MELONDS_INSOCKET_H
