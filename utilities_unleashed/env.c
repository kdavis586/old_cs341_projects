/**
 * utilities_unleashed
 * CS 341 - Fall 2022
 */

#include "format.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        print_env_usage();
        exit(1);
    }

    pid_t child = fork();

    if (child == -1) {
        print_fork_failed();
    }

    if (child == 0) {
        size_t i = 1;
        while (argv[i] && strcmp(argv[i], "--") != 0) {
            // Processing key value pairs
            char * input_copy = malloc(sizeof(argv[i]) + 1);
            strcpy(input_copy, argv[i]);
            const char * delim = "=";
            char * key = strtok(input_copy, delim);
            char * value = input_copy;
            
            
            if (!key || !value) {
                // Value does not parsed fail
                exit(1);
            }

            if (setenv(key, value, 1) == -1) {
                free(input_copy);
                exit(1);
            }
            free(input_copy);

            i++;
        }

        if ((int) i == argc) {
            // went through all argumments, no -- found
            print_env_usage();
            exit(1);
        }

        i++; // argv[i] was "--", increment to get to the exec args
        if (!argv[i]) {
            // no command found
            print_env_usage();
            exit(1);
        }
        execvp(argv[i], &argv[i]);
        print_exec_failed();
        exit(1);
    } else {
        int status;
        pid_t pid = waitpid(child, &status, 0);

        if (pid == -1) {
            exit(1);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            exit(1);
        }
    }

    return 0;
}
