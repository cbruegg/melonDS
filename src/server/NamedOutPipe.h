//
// Created by mail on 08/05/2020.
//

#ifndef MELONDS_NAMEDOUTPIPE_H
#define MELONDS_NAMEDOUTPIPE_H


#include <cstdio>
#include "../types.h"

class NamedOutPipe {
public:
    explicit NamedOutPipe(const char* name);

    void writeData(u32* data, size_t len);

    void writeData(int16_t* data, size_t len);

    void closePipe() const;

    void flushPipe();
private:
    int fd;

    void openIfRequired();

    const char *name;
};


#endif //MELONDS_NAMEDOUTPIPE_H
