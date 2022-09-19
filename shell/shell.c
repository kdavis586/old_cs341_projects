/**
 * shell
 * CS 341 - Fall 2022
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



// Forward declare helper functions
ssize_t _get_input(char ** buffer, size_t * size);
void _cleanup();
bool _parse_arguments(int argc, char * argv[]);
bool _validate_options(int argc, char * argv[], char ** hist_path, char ** script_path);

typedef struct process {
    char *command;
    pid_t pid;
} process;

// Globals that will be used for the duration of the shell
static char * CWD; // Current working directory

static char * INPUT_BUFFER; // Used for getting the user input
static size_t INPUT_SIZE;   // Used for getting the user input

static vector * CMD_HIST;   // Used to store the history of commands used during the shell's lifetime

static unsigned int MODE;   // indicates which arguments were passed (0 -> none, 1 -> -h, 2 -> -f, 3, -h and -f)
static FILE * HISTORY_FILE; // File for logging the history (if -h)
static FILE * SCRIPT_FILE;  // File to run commands from (if -f)

int shell(int argc, char *argv[]) {
    // Init variables that will be used for the lifetime of the shell
    opterr = 0; // No error reporting on getopt
    pid_t shell_pid = getpid();
    CWD = get_full_path(".");
    CMD_HIST = vector_create(&string_copy_constructor, &string_destructor, &string_default_constructor);

    // check for optional arguments
    if (!_parse_arguments(argc, argv)) {
        return 1;
    }

    signal(SIGINT, SIG_DFL); // TODO: Redirect SIGING (Ctrl + C) to kill the currently running foreground process
    while(true) {
        // Print shell prompt line
        print_prompt(CWD, shell_pid);

        // Read the user input and check to make sure it worked
        ssize_t chars = _get_input(&INPUT_BUFFER, &INPUT_SIZE);
        if (chars == -1 && errno != 0) {
            // getline failed
            _cleanup();
            return 1;
        } else if (chars > 0) {
            // Add the input into a history vector
            if (strcmp(INPUT_BUFFER, "exit\n") != 0) {
                // Push back with the newline character for proper writing
                vector_push_back(CMD_HIST, INPUT_BUFFER);
            }

            // replace the newline character with char NUL
            if (INPUT_BUFFER[chars - 1] == '\n') {
                INPUT_BUFFER[chars - 1] = '\0';
            }
        }

        if (strcmp(INPUT_BUFFER, "exit") == 0 || chars == -1) {
            /*
            * exit was input, free memory and exit the shell.
            * or
            * chars == -1 from getline WITHOUT errno being set to a valid getline error value
            */
            size_t i;
            if (MODE == 1 || MODE == 3) {
                for (i = 0; i < vector_size(CMD_HIST); i++) {
                    fputs((char *)*vector_at(CMD_HIST, i), HISTORY_FILE);
                }
            }
            _cleanup();
            break;
        }

    }
    return 0;
}

// Gets the input line from user
ssize_t _get_input(char ** buffer, size_t * size) {
    free(*buffer);
    *buffer = NULL;
    *size = 0;
    return getline(buffer, size, stdin);
}

// Frees all manually allocated memory
void _cleanup() {
    free(CWD);
    free(INPUT_BUFFER);
    vector_destroy(CMD_HIST);
}

// Parses arguments passed with starting the shell, checks them for validity and sets global variables accordingly
bool _parse_arguments(int argc, char * argv[]) {
    if (argc > 5) {
        // Too many arguments passed, max is 5 for: program_name option1 filename1 option2 filename2
        print_usage();
        return false;
    }

    char * hist_path = NULL;
    char * script_path = NULL;
    
    if (!_validate_options(argc, argv, &hist_path, &script_path)) {
        return false;
    }

    if (hist_path && !(HISTORY_FILE = fopen(hist_path, "a"))) {
        // opening/creating history file failed
        return false;
    }
    
    if (script_path && !(SCRIPT_FILE = fopen(script_path, "r"))) {
        print_script_file_error();
        return false;
    }

    if (HISTORY_FILE && SCRIPT_FILE) {
        MODE = 3;
    } else if (SCRIPT_FILE) {
        MODE = 2;
    } else if (HISTORY_FILE) {
        MODE = 1;
    }
    
    return true;
}

// Validates that the options passed into argv are valid and expected
bool _validate_options(int argc, char * argv[], char ** hist_path, char ** script_path) {
    int opt;
    while((opt = getopt(argc, argv, "h:f:")) != -1) {
        switch(opt) {
            case 'h':
                if (*hist_path) {
                    // History file was already set, same option twice
                    print_usage();
                    return false;
                }

                *hist_path = optarg;
                break;
            case 'f':
                if (*script_path) {
                    // Script file was already set, same option twice
                    print_usage();
                    return false;
                }
                
                *script_path = optarg;
                break;
            case '?':
                print_usage();
                return false;
        }
    }
    // Check that no non-opt args were passed
    if (argv[optind] != NULL) {
        print_usage();
        return false;
    }

    return true;
}
