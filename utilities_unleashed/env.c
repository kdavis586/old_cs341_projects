/**
 * utilities_unleashed
 * CS 341 - Fall 2022
 */

#include "format.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


extern char** environ;

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        print_env_usage();
        exit(1);
    }

    size_t i = 1;
    while (argv[i] && strcmp(argv[i], "--") != 0) {
        // Processing key value pairs
        char * input_copy = malloc(sizeof(argv[i]) + 1);
        strcpy(input_copy, argv[i]);

        char * delim = "=";
        char * key = strtok(input_copy, delim);
        char * value = strtok(NULL, delim);

        printf("Key: %s, Value: %s\n", key, value);
        if (!key || !value) {
            // Value does not parsed fail
            exit(1);
        }

        i++;
    }

    if ((int) i == argc) {
        // went through all argumments, no -- found
        exit(1);
    }

    return 0;
}
