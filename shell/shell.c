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

// Define process struct
typedef struct process {
    char *command;
    pid_t pid;
} process;

// Forward declare helper functions
void _shell_setup(pid_t * shell_pid, int argc, char * argv[]);
ssize_t _get_input(char ** buffer, size_t * size);
void _cleanup();
bool _parse_arguments(int argc, char * argv[]);
bool _validate_options(int argc, char * argv[], char ** hist_path, char ** script_path);
bool _run_builtin(char * command, vector * args);
void _print_history();
void _handle_get_input(ssize_t chars);
void _exit_success();
void _handle_exit();
bool _handle_history(char * command, vector * args);
bool _handle_prev_command(char * command, vector * args);
bool _handle_cd(char * command, vector * args);
bool _handle_prefix(char * command);
bool _handle_ps(char * command, vector * args);
void _add_to_history(char * command);
void _run_command(char * command);
bool _run_external(char * command, bool print_cmd);
vector * _get_commands(char ** command, char * separator);
void _kill_foreground();
void _create_process(pid_t pid, char * command, bool background);
bool _is_background_command(char * command);
void _handle_background_processes(char * command);
void _remove_process(pid_t pid);
void _list_processes();
void _populate_process_info(process_info * pinfo, process * prcss);
void _free_process_info(process_info * pinfo);
char * _get_proc_pid_path(process * prcss);
char * _create_run_time_string(unsigned long utime, unsigned long stime);
char * _create_start_time_string(unsigned long long start_time);
void _handle_output_funct(char * input_command, char * file_path);

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
static vector * PROCESSES;

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
    PROCESSES = shallow_vector_create();

    
    *shell_pid = getpid();

    if (!_parse_arguments(argc, argv)) {
        exit(1);
    }

    if (RUN_SCRIPT) {
        INPUT_STREAM = SCRIPT_FILE;
    }

    signal(SIGINT, _kill_foreground);
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

    vector_destroy(PROCESSES); // TODO: This does not free internal children, will cause leak
    PROCESSES = NULL; 

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
    if (_handle_ps(command, args)) return true;

    return false;
}

void _print_history() {
    size_t i;
    for (i = 0; i < vector_size(CMD_HIST); i++) {
        char * command = (char *)*vector_at(CMD_HIST, i);
        command[strlen(command) - 1] = '\0';
        print_history_line(i, command);
        command[strlen(command)] = '\n';
    }
}

// Record history (if specified), cleanup, and exit successfully from shell
void _exit_success() {
    size_t i;
    if (RECORD_HIST) {
        for (i = 0; i < vector_size(CMD_HIST); i++) {
            char * command = (char *)*vector_at(CMD_HIST, i);
            fputs(command, HISTORY_FILE);
        }
    }

    _cleanup();
    exit(0);
}

// If user types "exit" exit successfully
void _handle_exit() {
    if (strcmp(INPUT_BUFFER, "exit\n") == 0) {
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
        if (*command == '#') {
            if (vector_size(args) > 1) {
                print_invalid_command(command);
                return true;
            }

            char * number = malloc(sizeof(command));
            strcpy(number, command + 1);
            char * itr = number;
            while(*itr) {
                if (!isdigit(*itr)) {
                    print_invalid_command(command);
                    free(number);
                    return true;
                }

                itr++;
            }

            int n = atoi(number); // TODO: accepts "01" as 1, maybe bad...
            free(number);
            if (n < 0 || (size_t) n >= vector_size(CMD_HIST)) {
                print_invalid_index();
            } else {
                char * history_command = (char *)*vector_at(CMD_HIST, n);
                char * newline = strchr(history_command, '\n');
                if (newline) {
                    *newline = '\0';
                }
                print_command(history_command);
                _run_command(history_command);
                *newline = '\n';
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

// handles if user types !<prefix>
bool _handle_prefix(char * command) {
    if (*command == '!') {
        char * prefix = malloc(strlen(command));
        strcpy(prefix, command+1);
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
                    char * newline = strchr(history_command, '\n');
                    if (newline) {
                        *newline = '\0';
                    }
                    print_command(history_command);
                    _run_command(history_command);
                    *newline = '\n';
                    match = true;
                    break;
                }
            }
        }

        // no matches
        if (!match) print_no_history_match();
        free(prefix_compare);
        free(prefix);
        return true;
    }
    
    return false;
}

// handles ps
bool _handle_ps(char * command, vector * args) {
    if (vector_size(args) > 0) {
        char * ps = vector_get(args, 0);

        if (strcmp(ps, "ps") == 0) {
            if (vector_size(args) > 1) {
                print_invalid_command(command);
            } else {
                print_process_info_header();
                _list_processes();
                _add_to_history(command);
            }
            return true;
        }
    }
    return false;
}

// Adds command to history with a newline character
void _add_to_history(char * command) {
    command[strlen(command)] = '\n';
    vector_push_back(CMD_HIST, command);
}

// run command externally by forking, returns whether or not the command ran successfully
bool _run_external(char * command, bool print_cmd) {
    bool success = true;
    fflush(stdout);
    pid_t child;

    if ((child = fork()) == -1) {
        print_fork_failed();
        success = false;
    } else {
        if (child != 0 && print_cmd) {
            print_command_executed(child);  
        }
        
        bool background = _is_background_command(command);

        if (child == 0) {
            // Child process

            // Get rid of ampersand before parsing
            if (background) {
                char * ampersand = strchr(command, '&');
                *ampersand = '\0';
            }
            
            wordexp_t fake_argvc;
            if (wordexp(command, &fake_argvc, 0) != 0) {
                exit(1);
            }

            char ** fake_argv = fake_argvc.we_wordv;

            //_cleanup(); // TODO: Do I need this here?
            execvp(fake_argv[0], fake_argv);
            wordfree(&fake_argvc);
            exit(1);
        } else {
            _create_process(child, command, background);
            if (!background) {
                int status;
                pid_t pid = waitpid(child, &status, 0);

                if (pid == -1) {
                    print_wait_failed();
                    success = false;
                } else if ((WIFSIGNALED(status) && WTERMSIG(status) != SIGINT)|| (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
                    print_exec_failed(command);
                    success = false;
                }

                _remove_process(child);
            }
        }
    }

    return success;
}

// Run a general command.f
void _run_command(char * command) {
    // Handle background processes
    _handle_background_processes(command);  

    char * newline = strchr(command, '\n');
    if (newline) {
        *newline = '\0';
    }
    
    sstring * sstr_command = cstr_to_sstring(command);
    vector * args = sstring_split(sstr_command, ' '); // used for _run_builtin 

    if(!_run_builtin(command, args)) {
        // check for logical operators
        vector * commands = NULL;
        char * cmd1 = NULL;
        char * cmd2 = NULL;
        if ((commands = _get_commands(&command, "&&"))) {
            cmd1 = (char*)*vector_at(commands, 0);
            cmd2 = (char*)*vector_at(commands, 1);

            if (_run_external(cmd1, true)) {
                _run_external(cmd2, true);
            }

        } else if ((commands = _get_commands(&command, "||"))) {
            cmd1 = (char*)*vector_at(commands, 0);
            cmd2 = (char*)*vector_at(commands, 1);

            if (!_run_external(cmd1, true)) {
                _run_external(cmd2, true);
            }

        } else if ((commands = _get_commands(&command, ";"))) {
            cmd1 = (char*)*vector_at(commands, 0);
            cmd2 = (char*)*vector_at(commands, 1);

            _run_external(cmd1, true);
            _run_external(cmd2, true);
        } else if ((commands = _get_commands(&command, ">"))) {
            char * input_command = vector_get(commands, 0);
            char * file_path = vector_get(commands, 1);

            _handle_output_funct(input_command, file_path);
            _add_to_history(command);
        }
        
        
        // TODO: Reconnect this
        else {
            _run_external(command, true);
        }

        if (commands) vector_destroy(commands);
        _add_to_history(command);
    }

    if (RUN_SCRIPT) {
        print_command(command);
    }

    sstring_destroy(sstr_command);
    vector_destroy(args);  
}

// essentially splits input command on separator once, then returns results inside vector
vector * _get_commands(char ** command, char * separator) {
    char * sep_position = strstr(*command, separator);

    if (!sep_position) {
        return NULL;
    }

    vector * commands = string_vector_create();

    size_t cmd1_len = sep_position - *command;
    size_t cmd2_len = (*command + strlen(*command)) - (sep_position + strlen(separator));
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

// Kills forground child
void _kill_foreground() {
    
}

// Creates a process struct and stores it in PROCESSES
void _create_process(pid_t pid, char * command, bool background) {
    // create a copy of command to store
    char * command_copy = strdup(command);

    process * new_process = (process *) malloc(sizeof(process));
    // TODO: Add some logic about process groups if the process is a background process
    new_process->command = command_copy;
    new_process->pid = pid;

    vector_push_back(PROCESSES, new_process);
}

// Checks to see if command is suffixed with a "&", if so, it modifies the command to exclude the "&"
bool _is_background_command(char * command) {
    sstring * cmd_sstr = cstr_to_sstring(command);
    vector * split = sstring_split(cmd_sstr, ' ');
    char * last_arg = vector_get(split, vector_size(split) - 1);

    bool is_background = strcmp(last_arg, "&") == 0;
    // if (is_background) {
    //     char * ampersand = strchr(command, '&');
    //     *ampersand = '\0';
    // }
    sstring_destroy(cmd_sstr);
    vector_destroy(split);

    return is_background;
}

void _handle_background_processes(char * command) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == -1 && vector_size(PROCESSES) != 0) {
        print_wait_failed();
    } else if (pid != 0) {
        _remove_process(pid);

        if ((WIFSIGNALED(status) && WTERMSIG(status) != SIGINT)|| (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
            print_exec_failed(command);
        }
    }
}

void _remove_process(pid_t pid) {
    size_t i;
    for (i = 0; i < vector_size(PROCESSES); i++) {
        process * prcss = vector_get(PROCESSES, i);
        if (prcss->pid == pid) {
            vector_erase(PROCESSES, i);
        }
    }
}

void _list_processes() {
    size_t i;

    for (i = 0; i < vector_size(PROCESSES); i++) {
        process * prcss = vector_get(PROCESSES, i);
        process_info * pinfo = (process_info *) malloc(sizeof(process_info));
        _populate_process_info(pinfo, prcss);
        print_process_info(pinfo);
        _free_process_info(pinfo);
    }
}

void _populate_process_info(process_info * pinfo, process * prcss) {
    char * pid_path = _get_proc_pid_path(prcss);

    // Open proc for reading and free the path char *
    FILE * fd;
    if (!(fd = fopen(pid_path, "r"))) {
        // should never go here
        exit(2);
    }
    free(pid_path);

    // Setup needed variables for value storage
    long int nthreads;
    unsigned long virtual_size; 
    char state;
    unsigned long long start_time;
    unsigned long utime;
    unsigned long stime;
    
    // Ignore all unwanted values via '*', store the desired values accordingly
    int count = fscanf(fd, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld %*ld %llu %lu", &state, &utime, &stime, &nthreads, &start_time, &virtual_size);
    if (count != 6) {
        // shouldn't go here ever
        exit(2);
    }
    fclose(fd);

    // Get clock time into units that are not ticks
    char * time_str = _create_run_time_string(utime, stime);

    // start time adjustment
    char * start_str = _create_start_time_string(start_time);

    pinfo->pid = prcss->pid;
    pinfo->nthreads = nthreads;
    pinfo->vsize = virtual_size / 1024;
    pinfo->state = state;           // TODO: Populate all of this information from /proc/<pid>/stat
    pinfo->start_str = start_str;
    pinfo->time_str = time_str;
    pinfo->command = strdup(prcss->command);
}

char * _get_proc_pid_path(process * prcss) {
    sstring * sstr_path = cstr_to_sstring("/proc/");

    // Get length of number in char * form, allocate for it, store it, then turn into sstring for appending later
    int pid_length = snprintf(NULL, 0, "%d", prcss->pid);
    char * pid_cstr = malloc(pid_length + 1);
    snprintf(pid_cstr, pid_length + 1, "%d", prcss->pid);
    sstring * pid_sstr = cstr_to_sstring(pid_cstr);

    sstring * sstring_stat = cstr_to_sstring("/stat");

    // Append the path together and turn into cstr
    sstring_append(sstr_path, pid_sstr);
    sstring_append(sstr_path, sstring_stat);
    char * pid_path = sstring_to_cstr(sstr_path);

    // Free all resources used for the cstring creation
    sstring_destroy(sstr_path);
    sstring_destroy(pid_sstr);
    sstring_destroy(sstring_stat);
    free(pid_cstr);

    return pid_path;
}


void _free_process_info(process_info * pinfo) {
    free(pinfo->start_str);
    free(pinfo->time_str);
    free(pinfo->command);
    free(pinfo);
    pinfo = NULL;
}

char * _create_run_time_string(unsigned long utime, unsigned long stime) {
    utime /= sysconf(_SC_CLK_TCK);
    stime /= sysconf(_SC_CLK_TCK);
    size_t cpu_time = (size_t)(utime + stime);
    size_t minutes = cpu_time / 60;
    size_t seconds = cpu_time % 60;

    // Allocate the right amount of space for the time string using snprintf
    size_t minutes_len = snprintf(NULL, 0, "%zu", minutes);
    size_t seconds_len = snprintf(NULL, 0, "%zu", seconds);

    // allocate enough space for extra 0, :, and NUL (\0)
    char * time_str = malloc(minutes_len + seconds_len + 3);
    if (seconds < 10) {
        snprintf(time_str, minutes_len + seconds_len + 3, "%zu:0%zu", minutes, seconds);
    } else {
        snprintf(time_str, minutes_len + seconds_len + 2, "%zu:%zu", minutes, seconds);
    }

    return time_str;
}

char * _create_start_time_string(unsigned long long start_time) {
    start_time /= sysconf(_SC_CLK_TCK); // now in seconds

    // Get system uptime
    FILE * fd;
    if (!(fd = fopen("/proc/uptime", "r"))) {
        // should never go here
        exit(2);
    }
    unsigned long long uptime;
    fscanf(fd, "%llu %*llu", &uptime);
    fclose(fd);

    start_time += uptime;
    size_t start_hours = (size_t)(start_time / (60 * 60));
    size_t start_minutes = (size_t)(start_time / 60 % 60);

    // Allocate the right amount of space for the time string using snprintf
    int start_h_len = snprintf(NULL, 0, "%zu", start_hours);
    int start_m_len = snprintf(NULL, 0, "%zu", start_minutes);

    // allocate enough space for extra 0, :, and NUL (\0)
    size_t start_str_len = (start_h_len + start_m_len);
    char * start_str = malloc(start_str_len+ 3);
    if (start_minutes < 10) {
        snprintf(start_str, start_str_len + 3, "%zu:0%zu", start_hours, start_minutes);
    } else {
        snprintf(start_str, start_str_len + 2, "%zu:%zu", start_hours, start_minutes);
    }

    return start_str;
}

void _handle_output_funct(char * input_command, char * file_path) {
    // concept -> redirect stdout to file descriptor, then revert it
    print_command(input_command);
    int stdout_fd = dup(1);
    close(1);

    FILE * fd;
    if (!(fd = fopen(file_path, "w"))) {
        // should not go here
        fprintf(stderr, "here");
        exit(2);
    }

    _run_external(input_command, false);
    fclose(fd);
    dup2(stdout_fd, 1);
}
