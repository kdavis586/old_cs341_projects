/**
 * shell
 * CS 341 - Fall 2022
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <errno.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



// Forward declare helper functions
void _shell_setup(pid_t * shell_pid, int argc, char * argv[]);
ssize_t _get_input(char ** buffer, size_t * size);
void _cleanup();
bool _parse_arguments(int argc, char * argv[]);
bool _validate_options(int argc, char * argv[], char ** hist_path, char ** script_path);
bool _run_builtin(vector * args);
void _print_history();
char * _run_prefix(char * prefix);
char * _run_command(vector * args);
vector * _split_input(char * command);
void _handle_get_input(ssize_t chars);
void _exit_success();
void _handle_exit();
bool _handle_history(vector * args);
bool _handle_prev_command(vector * args);

// Define process struct
typedef struct process {
    char *command;
    pid_t pid;
} process;

// Globals that will be used for the duration of the shell
static char * CWD; // Current working directory
static char * INPUT_BUFFER; // Used for getting the user input
static size_t INPUT_SIZE;   // Used for getting the user input
static vector * CMD_HIST;   // Used to store the history of commands used during the shell's lifetime
static bool RECORD_HIST;
static bool RUN_SCRIPT;
static FILE * HISTORY_FILE; // File for logging the history (if -h)
static FILE * SCRIPT_FILE;  // File to run commands from (if -f)
static FILE * INPUT_STREAM; // Used to store the stream to read commands from


/*
 *  MAIN SHELL FUNCTION
*/
int shell(int argc, char *argv[]) {
    pid_t shell_pid;
    _shell_setup(&shell_pid, argc, argv);

    while(true) {
        print_prompt(CWD, shell_pid);
        ssize_t chars = _get_input(&INPUT_BUFFER, &INPUT_SIZE);
        _handle_get_input(chars);

        // chars guarenteed to be >= 0
        if (chars > 0) {
            // Format for running command
            if (INPUT_BUFFER[chars - 1] == '\n') {
                INPUT_BUFFER[chars - 1] = '\0';
            }
            _handle_exit();

            
            vector * command_args = _split_input(INPUT_BUFFER); // used for builtin
            // TODO: get rid of this
            vector_size(command_args);
            
            if (RUN_SCRIPT) {
                print_command(INPUT_BUFFER);
            }
            //vector_destroy(command_args) // TODO: Very bad to free every call, use same vector and clear
        }

    }

    // Should not go here
    exit(0);
}


/*
 * HELPER FUNCTIONS FOR SHELL
*/

// Sets some initials globals and gets the pid for the process running the main shell
//  NOTE: All functions used either do not cause errors (according to man) or are given functions
void _shell_setup(pid_t * shell_pid, int argc, char * argv[]) {
    opterr = 0; // No error reporting on getopt
    INPUT_STREAM = stdin;
    CWD = get_full_path(".");
    CMD_HIST = string_vector_create();
    *shell_pid = getpid();

    if (!_parse_arguments(argc, argv)) {
        exit(1);
    }

    if (RUN_SCRIPT) {
        INPUT_STREAM = SCRIPT_FILE;
    }

    signal(SIGINT, SIG_IGN); // TODO: Redirect SIGINT (Ctrl + C) to kill the currently running foreground process
}

// Parses arguments passed with starting the shell,
//  checks them for validity and sets global variables accordingly
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
        print_history_file_error();
        return false;
    }
    
    if (script_path && !(SCRIPT_FILE = fopen(script_path, "r"))) {
        print_script_file_error();
        return false;
    }

    if (HISTORY_FILE) {
        RECORD_HIST = true;
    }
    if (SCRIPT_FILE) {
        RUN_SCRIPT = true;
    }
    
    return true;
}

// Gets the input line from user
ssize_t _get_input(char ** buffer, size_t * size) {
    free(*buffer);
    *buffer = NULL;
    *size = 0;
    return getline(buffer, size, INPUT_STREAM);
}

// Handles when _get_input returns -1 (either fail for EOF), or nothing if chars recieved >= 0
void _handle_get_input(ssize_t chars) {
    if (chars != -1) return;

    if (errno != 0) {
        // _get_input failed
        _cleanup();
        exit(1);
    }

    // errno was 0 and chars == -1, EOF (Ctrl + D) was recieved
    _exit_success();
}

// Frees all manually allocated memory
void _cleanup() {
    free(CWD);
    free(INPUT_BUFFER);
    vector_destroy(CMD_HIST);
    free(HISTORY_FILE);
    free(SCRIPT_FILE);
    if (INPUT_STREAM != stdin) {
        free(INPUT_STREAM);
    }
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

/* will run builtin command if command is a built in
 * -1 if builtin  was not run
 * NULL if builtin run, but no command is to be saved
 * Other strings will be command to save
*/
bool _run_builtin(vector * args) {
    char path[PATH_MAX];
    char prefix[PATH_MAX];
    size_t n = 0;
    

    if (vector_size(args) == 0) return false;
    if (_handle_history(args)) return true;
    if (_handle_prev_command(args)) return true;
    else if (sscanf(command, "#%zu", &n) == 1) {
        if (n >= vector_size(CMD_HIST)) {
            print_invalid_index();
        } else  {
            //char * history_command = (char *)*vector_at(CMD_HIST, n);
            // TODO: EXECUTE COMMAND HERE
        }
        return true;
    } else if (sscanf(command, "!%s", prefix) == 1) {
        return _run_prefix(prefix);
        return true;
    }
    if (sscanf(INPUT_BUFFER, "cd %s", path) == 1) {
        if (chdir(path) == -1) {
            // TODO: Maybe base printing off of the specific errno
            print_no_directory(path);
        } else {
            free(CWD);
            CWD = NULL;
            CWD = get_full_path(".");
        }   
        
        return "1";
    }

    return false;
}

void _print_history() {
    size_t i;
    for (i = 0; i < vector_size(CMD_HIST); i++) {
        print_history_line(i, (char *)*vector_at(CMD_HIST, i));
    }
}

char * _run_prefix(char * prefix) {
    size_t prefix_len = strlen(prefix);
    size_t i;
    for (i = vector_size(CMD_HIST) - 1; i >= 0; i--) {
        char * command = (char *)*vector_at(CMD_HIST, i);
        char * prefix_compare = malloc(prefix_len + 1);
        strncpy(prefix_compare, command, prefix_len);
        prefix_compare[prefix_len] = '\0';

        bool match = !strcmp(prefix, prefix_compare);
        free(prefix_compare);
        prefix_compare = NULL;
        if (match) {
            // TODO: EXECUTE command
            return command;
        }
    }

    print_no_history_match();
    return  NULL;
}

// char * _run_command(vector * args) {
//     // TODO: DOES NOT WORK, NEED list of args
//     char * to_save = _run_builtin(args);
//     if (to_save != (char *)-1) {
//         return to_save;
//     }

//     // command was not builtin, run with fork exec wait
//     return NULL;
// }

vector * _split_input(char * command) {
    vector * command_args = string_vector_create();
    char * arg = strtok(command, " ");
    if (arg) {
        vector_push_back(command_args, arg);
    }

    while ((arg = strtok(NULL, " "))) {
        vector_push_back(command_args, arg);
    }

    vector_push_back(command_args, NULL);
    
    // TODO: remove this
    size_t i;
    for (i = 0; i < vector_size(command_args); i++) {
        fprintf(stderr, "%s\n", (char*)*vector_at(command_args, i));
    }  
    return command_args;
}

// Record history (if specified), cleanup, and exit successfully from shell
void _exit_success() {
    size_t i;
    if (RECORD_HIST) {
        for (i = 0; i < vector_size(CMD_HIST); i++) {
            fputs((char *)*vector_at(CMD_HIST, i), HISTORY_FILE);
        }
    }

    _cleanup();
    exit(0);
}

// If user types "exit" exit successfully
void _handle_exit() {
    if (strcmp(INPUT_BUFFER, "exit") == 0) {
        _exit_success();
    }
}

bool _handle_history(vector * args) {
    if (vector_size(args) == 1) {
        char * command = (char *)*vector_at(args, 0);
        if (strcmp(command, "!history") == 0) {
            _print_history();
            return true;
        }
    }
    
    return false;
}

bool _handle_prev_command(vector * args) {
    if (vector_size(args) == 1) {
        int n;
        char * command = (char *)*vector_at(args, 0);
        if (sscanf(command "#%d", n) == 1) {
            if (n < 0 || n >= vector_size(CMD_HIST)) {
                print_invalid_index();
            } else {
                char * history_command = (char *)*vector_at(CMD_HIST, n);
                // TODO: EXECUTE history_command
                vector_push_back(CMD_HIST, history_command);
            }
            return true;
        }
    }

    return false;
}