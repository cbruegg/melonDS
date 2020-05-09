//
// Created by mail on 08/05/2020.
//

#include <sys/stat.h>
#include <fcntl.h>
#include <zconf.h>
#include "NamedOutPipe.h"

// TODO Error handling

NamedOutPipe::NamedOutPipe(const char *name) {
    mkfifo(name, 0666);
    this->name = name;
    this->fd = -1;
}

void NamedOutPipe::writeData(u32 *data, size_t len) {
    this->openIfRequired();
    write(this->fd, data, len);
}

void NamedOutPipe::writeData(int16_t *data, size_t len) {
    this->openIfRequired();
    write(this->fd, data, len);
}

void NamedOutPipe::closePipe() const {
    close(this->fd);
}

void NamedOutPipe::flushPipe() {
    // TODO ?
}

void NamedOutPipe::openIfRequired() {
    if (this->fd == -1) {
        this->fd = open(this->name, O_WRONLY);
    }
}
