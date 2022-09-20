/**
 * shell
 * CS 341 - Fall 2022
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <ctype.h>
#include <errno.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>



// Forward declare helper functions
void _shell_setup(pid_t * shell_pid, int argc, char * argv[]);
ssize_t _get_input(char ** buffer, size_t * size);
void _cleanup();
bool _parse_arguments(int argc, char * argv[]);
bool _validate_options(int argc, char * argv[], char ** hist_path, char ** script_path);
bool _run_builtin(char * command, vector * args);
void _print_history();
void _split_input(char * command);
void _handle_get_input(ssize_t chars);
void _exit_success();
void _handle_exit();
bool _handle_history(char * command, vector * args);
bool _handle_prev_command(char * command, vector * args);
bool _handle_cd(char * command, vector * args);
bool _handle_prefix(char * command);
void _add_to_history(char * command);
void _run_command(char * command);
bool _run_external(char * command);
vector * _get_commands(char ** command, char * separator);

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
static ssize_t CHARS_READ;

/*
 *  MAIN SHELL FUNCTION
*/
int shell(int argc, char *argv[]) {
    pid_t shell_pid;
    _shell_setup(&shell_pid, argc, argv);

    while(true) {
        print_prompt(CWD, shell_pid);
        CHARS_READ = _get_input(&INPUT_BUFFER, &INPUT_SIZE);
        _handle_get_input(CHARS_READ);

        // chars guarenteed to be >= 0
        if (CHARS_READ > 0) {
            // Format for running command
            if (INPUT_BUFFER[CHARS_READ - 1] == '\n') {
                INPUT_BUFFER[CHARS_READ - 1] = '\0';
            }
            _handle_exit();
            _run_command(INPUT_BUFFER);
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
    CWD = get_full_path(".");
    INPUT_BUFFER = NULL;
    INPUT_SIZE = 0;
    CMD_HIST = string_vector_create();
    RECORD_HIST = false;
    RUN_SCRIPT = false;
    HISTORY_FILE = NULL;
    SCRIPT_FILE = NULL;
    INPUT_STREAM = stdin;
    CHARS_READ = -1;
    
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

    if (hist_path) {
        HISTORY_FILE = fopen(hist_path, "a");
        if (!HISTORY_FILE) {
            print_history_file_error();
            return false;
        }
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
    CWD = NULL;

    free(INPUT_BUFFER);
    INPUT_BUFFER = NULL;

    vector_destroy(CMD_HIST);
    CMD_HIST = NULL;

    if (HISTORY_FILE) fclose(HISTORY_FILE);
    HISTORY_FILE = NULL;

    if (SCRIPT_FILE) fclose(SCRIPT_FILE);
    SCRIPT_FILE = NULL;
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

// returns whether or not a builtin function ran
bool _run_builtin(char * command, vector * args) {  
    if (vector_size(args) == 0) return false;
    if (_handle_history(command, args)) return true;
    if (_handle_prev_command(command, args)) return true;
    if (_handle_cd(command, args)) return true;
    if (_handle_prefix(command)) return true;

    return false;
}

void _print_history() {
    size_t i;
    for (i = 0; i < vector_size(CMD_HIST); i++) {
        char * command = (char *)*vector_at(CMD_HIST, i);
        print_history_line(i, command);
    }
}

// // splits the input of the command by spaces and stores the values in CMD_ARGS
// void _split_input(char * command) {
//     vector_clear(CMD_ARGS);
//     char * command_copy = malloc(strlen(command) + 1);
//     strcpy(command_copy, command);
//     char * arg = strtok(command_copy, " ");
//     if (arg) {
//         vector_push_back(CMD_ARGS, arg);
//     }

//     while ((arg = strtok(NULL, " "))) {
//         vector_push_back(CMD_ARGS, arg);
//     }
//     free(command_copy);
// }

// Record history (if specified), cleanup, and exit successfully from shell
void _exit_success() {
    size_t i;
    if (RECORD_HIST) {
        for (i = 0; i < vector_size(CMD_HIST); i++) {
            char * command = (char *)*vector_at(CMD_HIST, i);
            // char * newline = strchr(command, '\n');
            // if (!newline) {
            //     command[strlen(command)] = '\n';
            // }

            fputs(command, HISTORY_FILE);
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

// handles if user types "!history"
bool _handle_history(char * command, vector * args) {
    if (vector_size(args) > 0) {
        char * first_val = (char *)*vector_at(args, 0);
        if (strcmp(first_val, "!history") == 0) {
            if (vector_size(args) > 1) {
                print_invalid_command(command);
            } else {
                _print_history();
            }
            return true;
        }
    }
    
    return false;
}

// handles if user types #<n>
bool _handle_prev_command(char * command, vector * args) {
    if (vector_size(args) > 0) {
        int n;
        char * first_val = (char *)*vector_at(args, 0);
        if (sscanf(first_val, "#%d", &n) == 1) {
            if (vector_size(args) > 1) {
                print_invalid_command(command);
            } else {
                if (n < 0 || (size_t) n >= vector_size(CMD_HIST)) {
                    print_invalid_index();
                } else {
                    char * history_command = (char *)*vector_at(CMD_HIST, n);
                    _run_command(history_command);
                }
            }
            return true;
        }
    }

    return false;
}

// handles if user types cd <path>
bool _handle_cd(char * command, vector * args) {
    if (vector_size(args) > 0) {
        char * cd = (char *)*vector_at(args, 0);

        if (strcmp(cd, "cd") == 0) {
            if (vector_size(args) == 1 || vector_size(args) >= 3) {
                print_invalid_command(command);
            } else {
                char * path = (char *)*vector_at(args, 1);
                if (chdir(path) == -1) {
                    print_no_directory(path);
                }

                free(CWD);
                CWD = NULL;
                CWD = get_full_path(".");
            }
            _add_to_history(command); // adding input buffer because not segmented
            return true;
        }
    }
    return false;
}

// hanldes if user types !<prefix>
bool _handle_prefix(char * command) {
    char prefix[strlen(command)];

    if (sscanf(command, "!%s", prefix) == 1) {
        size_t prefix_len = strlen(prefix);
        char * prefix_compare = malloc(sizeof(prefix));
        size_t i;
        bool match = false;

        if (vector_size(CMD_HIST) > 0) {
            for (i = 0; i < vector_size(CMD_HIST); i++) {
                size_t idx = vector_size(CMD_HIST) - 1 - i;
                char * history_command = (char *)*vector_at(CMD_HIST, idx);
                prefix_compare[0] = '\0';
                strncpy(prefix_compare, history_command, prefix_len);
                prefix_compare[prefix_len] = '\0';
                
                if (!strcmp(prefix, prefix_compare)) {
                    _run_command(history_command);
                    match = true;
                    break;
                }
            }
        }

        // no matches
        if (!match) print_no_history_match();
        free(prefix_compare);
        return true;
    }
    
    return false;
}

// Adds command to history with a newline character
void _add_to_history(char * command) {
    if (command[CHARS_READ] == '\0') {
        command[CHARS_READ] = '\n';
    }
    //CHARS_READ = -1;
    vector_push_back(CMD_HIST, command);
}

// run command externally by forking, returns whether or not the command ran successfully
bool _run_external(char * command) {
    bool success = true;
    fflush(stdout);
    pid_t child;

    if ((child = fork()) == -1) {
        print_fork_failed();
        success = false;
    } else {
        if (child != 0) {
            print_command_executed(child);
        }
        
        if (child == 0) {
            // Child process
            wordexp_t fake_argvc;
            if (wordexp(command, &fake_argvc, 0) != 0) {
                exit(1);
            }

            char ** fake_argv = fake_argvc.we_wordv;
            _cleanup();
            execvp(fake_argv[0], fake_argv);
            exit(1);
        } else {
            int status;
            pid_t pid = waitpid(child, &status, 0);

            if (pid == -1) {
                print_wait_failed();
                success = false;
            } else if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
                print_exec_failed(command);
                success = false;
            }
        }
    }

    vector_push_back(CMD_HIST, command);
    return success;
}

// Run a general command.
void _run_command(char * command) {
    char * newline = strchr(command, '\n');
    if (newline) {
        *newline = '\0';
    }
    
    sstring * sstr_command = cstr_to_sstring(command);
    vector * args = sstring_split(sstr_command, ' '); // used for _run_builtin 
    sstring_destroy(sstr_command);

    if(!_run_builtin(command, args)) {
        // check for logical operators
        sstring * sstr_command = cstr_to_sstring(command);
        vector * semi_list = sstring_split(sstr_command, ';');
        sstring_destroy(sstr_command);

        vector * commands = NULL;
        char * cmd1 = NULL;
        char * cmd2 = NULL;
        if ((commands = _get_commands(&command, "&&"))) {
            cmd1 = (char*)*vector_at(commands, 0);
            cmd2 = (char*)*vector_at(commands, 1);

            if (_run_external(cmd1)) {
                _run_external(cmd2);
            }

        } else if ((commands = _get_commands(&command, "||"))) {
            cmd1 = (char*)*vector_at(commands, 0);
            cmd2 = (char*)*vector_at(commands, 1);

            if (!_run_external(cmd1)) {
                _run_external(cmd2);
            }

        } else if (vector_size(semi_list) > 1) {
            cmd1 = (char *)*vector_at(semi_list, 0);
            cmd2 = (char *)*vector_at(semi_list, 1);

            _run_external(cmd1);
            _run_external(cmd2);
        } else {
            fprintf(stderr, "command to be run: %s\n", command);
            _run_external(command);
        }

        vector_destroy(commands);
        vector_destroy(semi_list);
    }

    if (RUN_SCRIPT) {
        print_command(command);
    }

    vector_destroy(args);
}

vector * _get_commands(char ** command, char * separator) {
    char * sep_position = strstr(*command, separator);

    if (!sep_position) {
        return NULL;
    }

    vector * commands = string_vector_create();

    size_t cmd1_len = sep_position - *command;
    size_t cmd2_len = (*command + strlen(*command)) - (sep_position + strlen(separator));
    fprintf(stderr, "cmd1_len: %zu\n", cmd1_len);
    fprintf(stderr, "cmd2_len: %zu\n", cmd2_len);
    char * cmd1 = malloc(cmd1_len + 1);
    char * cmd2 = malloc(cmd2_len + 1);
    strncpy(cmd1, *command, cmd1_len);
    cmd1[cmd1_len] = '\0';
    char * cmd2_start = sep_position + strlen(separator);
    while (isspace(*cmd2_start)) cmd2_start++;
    strncpy(cmd2, cmd2_start, cmd2_len);
    cmd2[cmd2_len] = '\0';
    
    vector_push_back(commands, cmd1);
    vector_push_back(commands, cmd2);
    free(cmd1);
    free(cmd2);
    return commands;
}