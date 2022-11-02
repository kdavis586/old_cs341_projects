/**
 * charming_chatroom
 * CS 341 - Fall 2022
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READ_FAIL_NUM 2
#define WRITE_FAIL_NUM 3

static size_t read_counter;

int my_read(int fd, void *buff, size_t count) {
    read_counter++;

    if (read_counter == READ_FAIL_NUM) {
        read_counter = 0;

        if (rand() % 2 > 0) {
            return read(fd, buff, count / 2 + 1);
        } else {
            errno = EINTR;
            return -1;
        }
    }

    return read(fd, buff, count);
}

int my_write(int fd, const void *buff, size_t count) {
    read_counter++;

    if (read_counter == WRITE_FAIL_NUM) {
        read_counter = 0;

        if (rand() % 2 > 0) {
            return write(fd, buff, count / 2 + 1);
        } else {
            errno = EINTR;
            return -1;
        }
    }

    return write(fd, buff, count);
}
