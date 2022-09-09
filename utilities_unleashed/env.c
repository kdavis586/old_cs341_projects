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
            char * equal_ptr = NULL;
            if (!(equal_ptr = strchr(argv[i], '='))) {
                print_env_usage();
                exit(1);
            }
            // Processing key value pairs
            // char * input_copy = malloc(sizeof(argv[i]) + 1);
            // strcpy(input_copy, argv[i]);
            // const char * delim = "=";
            // char * key = strtok(input_copy, delim);
            // char * value = input_copy;

            // if (argv[i][0] == '=') {
            //     value = key;
            //     key = NULL;
            // }
            size_t key_len = equal_ptr - argv[i];
            char * key = malloc(key_len + 1);
            key[key_len] = '\0';
            strncpy(key, argv[i], key_len);

            size_t val_len = sizeof(equal_ptr + 1);
            char * value = malloc(val_len + 1);
            strcpy(value, equal_ptr + 1);

            if (value[0] == '%') {
                char * env_value = getenv(&value[1]);
                
                if (!env_value) {
                    free(key);
                    free(value);
                    exit(1);
                } else {
                    free(value);
                    value = malloc(sizeof(env_value) + 1);
                    strcpy(value, env_value);
                }
            }

            if (setenv(key, value, 1) == -1) {
                print_environment_change_failed();
                free(key);
                free(value);
                exit(1);
            }
            free(key);
            free(value);

            i++;
        }

        if ((int) i == argc) {
            // went through all arguments, no -- found
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

        if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
            exit(1);
        }
    }

    return 0;
}
