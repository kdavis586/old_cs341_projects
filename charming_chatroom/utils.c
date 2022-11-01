/**
 * charming_chatroom
 * CS 341 - Fall 2022
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1) {
        return read_bytes;
    }
        
    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    // Your code here
    int32_t converted = (int32_t)htonl(size);
    ssize_t written_bytes = 
        write_all_to_socket(socket, (char *)&converted, MESSAGE_SIZE_DIGITS);
    if (written_bytes == 0 || written_bytes == -1) {
        return written_bytes;
    }

    return written_bytes;
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    size_t total_read = 0;
    while (total_read < count) {
        ssize_t cur_read = read(socket, buffer, count);
        if (cur_read == 0) {
            return total_read;
        } else if (cur_read > 0) {
            total_read += (size_t)cur_read;
        } else if (cur_read == -1 && errno == SIGINT) {
            continue;
        } else {
            return -1;
        }
    }

    return total_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    size_t total_wrote = 0;
    while (total_wrote < count) {
        ssize_t cur_read = write(socket, buffer, count);
        if (cur_read == 0) {
            return total_wrote;
        } else if (cur_read > 0) {
            total_wrote += (size_t)cur_read;
        } else if (cur_read == -1 && errno == SIGINT) {
            continue;
        } else {
            return -1;
        }
    }

    return total_wrote;
}
