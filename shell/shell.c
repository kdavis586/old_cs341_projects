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

// Forward declare helper functions
ssize_t _get_input(char ** buffer, size_t * size);
void _cleanup();

typedef struct process {
    char *command;
    pid_t pid;
} process;

// Globals that will be used for the duration of the shell
static char * CWD;
static char * INPUT_BUFFER;
static size_t INPUT_SIZE;
static vector * CMD_HIST;

int shell(int argc, char *argv[]) {
    // Init variables that will be used for the lifetime of the shell
    pid_t shell_pid = getpid();
    CWD = get_full_path(".");
    CMD_HIST = vector_create(&string_copy_constructor, &string_destructor, &string_default_constructor);

    while(true) {
        // Print shell prompt line
        print_prompt(CWD, shell_pid);

        // Read the user input and check to make sure it worked
        ssize_t chars = _get_input(&INPUT_BUFFER, &INPUT_SIZE);
        if (chars == -1) {
            // getline failed
            _cleanup();
            return 1;
        } else if (chars > 0 && INPUT_BUFFER[chars - 1] == '\n') {
            // replace the newline character with char NUL
            INPUT_BUFFER[chars - 1] = '\0';

            // Add the input into a history vector
            vector_push_back(CMD_HIST, INPUT_BUFFER);
        }

        if (strcmp(INPUT_BUFFER, "exit") == 0) {
            // exit was input, free memory and exit the shell.
             _cleanup();
            break;
        }

    }
    return 0;
}

ssize_t _get_input(char ** buffer, size_t * size) {
    free(*buffer);
    *buffer = NULL;
    *size = 0;
    return getline(buffer, size, stdin);
}

void _cleanup() {
    free(CWD);
    free(INPUT_BUFFER);
    vector_destroy(CMD_HIST);
}
