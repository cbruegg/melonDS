//
// Created by mail on 08/05/2020.
//

#include <sys/stat.h>
#include <fcntl.h>
#include <zconf.h>
#include "NamedInPipe.h"

// TODO Error handling

NamedInPipe::NamedInPipe(const char *name) {
    mkfifo(name, 0666);
    this->name = name;
    this->fd = -1;
}

void NamedInPipe::readData(void *buf, size_t len) {
    this->openIfRequired();
    read(this->fd, buf, len);
}

void NamedInPipe::closePipe() const {
    close(this->fd);
}

void NamedInPipe::openIfRequired() {
    if (fd == -1) {
        this->fd = open(name, O_RDONLY);
    }
}
