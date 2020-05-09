//
// Created by mail on 08/05/2020.
//

#ifndef MELONDS_NAMEDINPIPE_H
#define MELONDS_NAMEDINPIPE_H


#include <cstdio>
#include "../types.h"

class NamedInPipe {
public:
    explicit NamedInPipe(const char *name);

    void readData(void *buf, size_t len);

    void closePipe() const;

private:
    int fd;
    const char *name;

    void openIfRequired();
};


#endif //MELONDS_NAMEDINPIPE_H
