/**
 * shell
 * CS 341 - Fall 2022
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct process {
    char *command;
    pid_t pid;
} process;

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    pid_t shell_pid = getpid();
    char * cwd = NULL;
    char * input_buffer = NULL;
    size_t input_size = 0;

    while(true) {
        // Get the current working directory
        free(cwd);
        cwd = get_full_path(".");

        // Print shell prompt line
        print_prompt(cwd, shell_pid);

        // Read the user input and check to make sure it worked
        free(input_buffer);
        input_buffer = NULL;
        input_size = 0;
        ssize_t chars = getline(&input_buffer, &input_size, stdin);
        if (chars == -1) {
            // getline failed
            free(cwd);
            free(input_buffer);
            return 1;
        } else if (chars > 0 && input_buffer[chars - 1] == '\n') {
            // replace the newline character with char NUL
            input_buffer[chars - 1] = '\0';
        }

        if (strcmp(input_buffer, "exit") == 0) {
            free(cwd);
            free(input_buffer);
            break;
        }

    }
    return 0;
}
