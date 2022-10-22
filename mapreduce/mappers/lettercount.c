/**
 * mapreduce
 * CS 341 - Fall 2022
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mapper.h"

void mapper(const char *data, FILE *output) {
    while (*data) {
        int c = *data++;
        if (isalpha(c)) {
            fprintf(output, "letters: 1\n");
        }
    }
}

MAKE_MAPPER_MAIN(mapper)
