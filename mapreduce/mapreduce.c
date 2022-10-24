/**
 * mapreduce
 * CS 341 - Fall 2022
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "utils.h"

// forward declarations
void close_other_pipes(size_t process_index, int ** pipe_list, size_t pipe_list_len);

int main(int argc, char **argv) {
    if (argc != 6) {
        print_usage();
        exit(1);
    }
    char * input_file = argv[1];
    char * output_file_name = argv[2];
    char * map_exc = argv[3];
    char * red_exc = argv[4];
    size_t mapper_count = 0;
    if (sscanf(argv[5], "%zu", &mapper_count) != 1) {
        fprintf(stderr, "Failed to get mapper count.\n");
        exit(1);
    }
    // Create an input pipe for each mapper.
    int * mapper_pipes[mapper_count];
    size_t i;
    for (i = 0; i < mapper_count; i++) {
        mapper_pipes[i] = malloc(sizeof(int) * 2);
        int success = pipe(mapper_pipes[i]);

        if (success == -1) {
            // error
            perror("Error making mapper pipe");
            exit(1);
        }
    }

    // Create one input pipe for the reducer.
    int reduce_pipe[2];
    int success = pipe(reduce_pipe);
    if (success == -1) {
        perror("Error making reducer pipe");
        exit(1);
    }

    // Open the output file.
    //FILE * output_fd = fopen(output_file, 'w');
    pid_t splitters[mapper_count];
    pid_t mappers[mapper_count];
    for (i = 0; i < mapper_count; i++) {
        char * splitter = "./splitter";
        #ifdef DEBUG
            splitter = "./splitter-debug";
        #endif

        pid_t splitter_child;
        if ((splitter_child = fork()) == -1) {
            perror("Failed to create splitter child");
            exit(1);
        }

        if (splitter_child == 0) {
            // close all pipes that are not index i
            close_other_pipes(i, mapper_pipes, mapper_count);
            close(reduce_pipe[0]);
            close(reduce_pipe[1]);

            // is child
            // we do not need to read here
            if (close(mapper_pipes[i][0]) == -1) {
                perror("Failed to close read side of pipe for splitter");
                exit(1);
            }
            // create command arr

            // turn numbers into strings
            char mapper_count_str[100];
            char index_str[100];
            snprintf(mapper_count_str, sizeof(mapper_count), "%zu", mapper_count);
            snprintf(index_str, sizeof(index_str), "%zu", i);

            char * command_arr[5] = {splitter, input_file, mapper_count_str, index_str, NULL};

            dup2(mapper_pipes[i][1], STDOUT_FILENO); // redirect pipe's stdout
            execv(splitter, command_arr);

            print_nonzero_exit_status(splitter, 1);
            exit(1);
        }
        splitters[i] = splitter_child;

        pid_t mapper_child;
        if ((mapper_child = fork()) == -1) {
            perror("Failed to create mapper child");
            exit(1);
        }

        if (mapper_child == 0) {
            // close all pipes that are not index i
            close_other_pipes(i, mapper_pipes, mapper_count);
            close(reduce_pipe[0]); // do not need to read from the reducer pipe

            // we do not need to write here
            if (close(mapper_pipes[i][1]) == -1) {
                perror("Failed to close read side of pipe for mapper");
                exit(1);
            }

            dup2(mapper_pipes[i][0], STDIN_FILENO); // read from the pipe via stdin -> mapper function will need this
            dup2(reduce_pipe[1], STDOUT_FILENO);

            char * command_arr[2] = {map_exc, NULL};
            execv(map_exc, command_arr);

            print_nonzero_exit_status(map_exc, 1);
            exit(1);
        }
        mappers[i] = mapper_child;
    }
    
    pid_t reduce_child;
    if ((reduce_child = fork()) == -1) {
        perror("Failed to create reduce child");
        exit(1);
    }

    // Start the reducer process.
    if (reduce_child == 0) {
        close_other_pipes(mapper_count + 1, mapper_pipes, mapper_count);
        close(reduce_pipe[1]); // won't need to write

        dup2(reduce_pipe[0], STDIN_FILENO);
        FILE * output_file = fopen(output_file_name, "w");
        if (!output_file) {
            perror("Failed to open file for writing");
            exit(1);
        }

        dup2(fileno(output_file), STDOUT_FILENO);

        char * command_arr[2] = {red_exc, NULL};
        execv(red_exc, command_arr);
        print_nonzero_exit_status(red_exc, 1);
        exit(1);
    }


    // close splitter and mapper pipes in the parent process
    close_other_pipes(mapper_count + 1, mapper_pipes, mapper_count);
    close(reduce_pipe[0]);
    close(reduce_pipe[1]);
    
    // wait on the child processes to finish
    for (i = 0; i < mapper_count; i++) {
        int splitter_status;
        waitpid(splitters[i], &splitter_status, 0);
        int mapper_status;
        waitpid(mappers[i], &mapper_status, 0);
        free(mapper_pipes[i]);
    }

    // Wait for the reducer to finish.
    int reduce_status;
    waitpid(reduce_child, &reduce_status, 0);
    // Print nonzero subprocess exit codes.

    // Count the number of lines in the output file.
    print_num_lines(output_file_name);
    return 0;
}

// if process index > pipe_list_len, close all pipes
void close_other_pipes(size_t process_index, int ** pipe_list, size_t pipe_list_len) {
    size_t i;
    for (i = 0; i < pipe_list_len; i++) {
        if (i != process_index || process_index > pipe_list_len) {
            close(pipe_list[i][0]);
            close(pipe_list[i][1]);
        }
    }
}
