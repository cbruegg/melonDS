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


#endif //MELONDS_INSOCKET_H
