/**
 * nonstop_networking
 * CS 341 - Fall 2022
 */
#include "common.h"
#include "errno.h"
#include <unistd.h>

// Write all bytes to the file descriptor, 0 success, -1 fail
int write_all(int fd, char * buffer, size_t size) {
    size_t bytes_wrote = 0;
    while (bytes_wrote != size) {
        ssize_t cur_written = write(fd, buffer + bytes_wrote, size - bytes_wrote);
        if (cur_written == 0) {
            return 0;
        } else if (cur_written > 0) {
            bytes_wrote += (size_t)cur_written;
        } else if (cur_written == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        } else {
            // error while writing
            return -1;
        }
    }

    return 0;
}

// Read all bytes from the file descriptor, 0 success, -1 fail
int read_all(int fd, char * buffer, size_t size) {
    size_t bytes_read = 0;
    while (bytes_read < size) {
        ssize_t cur_read = read(fd, buffer + bytes_read, size - bytes_read);
        if (cur_read == 0) {
            return 0;
        } else if (cur_read > 0) {
            bytes_read += (size_t)cur_read;
        } else if (cur_read == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}