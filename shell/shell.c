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
int _parse_arguments(int argc, char * argv[]);

typedef struct process {
    char *command;
    pid_t pid;
} process;

// Globals that will be used for the duration of the shell
static char * CWD;

static char * INPUT_BUFFER;
static size_t INPUT_SIZE;

static vector * CMD_HIST;

static unsigned int MODE;
static FILE * HISTORY_FILE;
static FILE * SCRIPT_FILE;

int shell(int argc, char *argv[]) {
    // Init variables that will be used for the lifetime of the shell
    opterr = 0; // No error reporting on getopt
    pid_t shell_pid = getpid();
    CWD = get_full_path(".");
    CMD_HIST = vector_create(&string_copy_constructor, &string_destructor, &string_default_constructor);

    // check for optional arguments
    if (_parse_arguments(argc, argv) == -1) {
        return 1;
    }

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

int _parse_arguments(int argc, char * argv[]) {
    // TODO: Handle the when arguments are passed without a flag
    char * hist_path = NULL;
    char * script_path = NULL;

    if (argc <= 5) {
        int opt;
        while((opt = getopt(argc, argv, "h:f:")) != -1) {
            switch(opt) {
                case 'h':
                    if (hist_path) {
                        // History file was already set, same option twice
                        print_usage();
                        return -1;
                    }

                    hist_path = optarg;
                    break;
                case 'f':
                    if (script_path) {
                        // Script file was already set, same option twice
                        print_usage();
                        return -1;
                    }
                    
                    script_path = optarg;
                    break;
                case '?':
                    print_usage();
                    return -1;
            }
        }
        // Check that no non-opt args were passed
        if (argv[optind] != NULL) {
            print_usage();
            return -1;
        }

        if (hist_path && !(HISTORY_FILE = fopen(hist_path, "w"))) {
            // opening/creating history file failed
            return -1;
        }
        
        if (script_path && !(SCRIPT_FILE = fopen(script_path, "r"))) {
            print_script_file_error();
            return -1;
        }

        if (HISTORY_FILE && SCRIPT_FILE) {
            MODE = 3;
        } else if (SCRIPT_FILE) {
            MODE = 2;
        } else if (HISTORY_FILE) {
            MODE = 1;
        }
        
        return 0;
    } 

    // Too many arguments passed, max is 5 for: program_name option1 filename1 option2 filename2
    print_usage();
    return -1;
}
