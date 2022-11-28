/**
 * nonstop_networking
 * CS 341 - Fall 2022
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

int read_all(int fd, char * buffer, size_t size);

int write_all(int fd, char * buffer, size_t size);
