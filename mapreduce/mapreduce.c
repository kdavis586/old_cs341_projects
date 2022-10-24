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

int main(int argc, char **argv) {
    if (argc != 6) {
        print_usage();
    }
    char * input_file = argv[1];
    // char * output_file = argv[2];
    // char * map_exc = argv[3];
    // char * red_exc = argv[4];
    size_t mapper_count = 0;
    if (sscanf(argv[5], "%zu", &mapper_count) != 1) {
        fprintf(stderr, "Failed to get mapper count.\n");
        exit(1);
    }
    // Create an input pipe for each mapper.
    int * mapper_pipes[mapper_count];
    size_t i;
    for (i = 0; i < mapper_count; i++) {
        int fh[2];
        int success = pipe(fh);

        if (success == -1) {
            // error
            perror("Error making mapper pipe");
            exit(1);
        }

        mapper_pipes[i] = fh;
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
            // is child
            // we do not need to read here
            if (close(mapper_pipes[i][0]) == -1) {
                perror("Failed to close read side of pipe for splitter");
                exit(1);
            }
            // create command arr
            char mapper_count_str[100];
            char index_str[100];
            snprintf(mapper_count_str, sizeof(mapper_count), "%zu", mapper_count);
            snprintf(index_str, sizeof(index_str), "%zu", i);
            char * command_arr[5] = {splitter, input_file, mapper_count_str, index_str, NULL};

            dup2(mapper_pipes[i][1], 1); // redirect pipe's stdout
            execv(splitter, command_arr);
            exit(1);
        }
        splitters[i] = splitter_child;

        pid_t mapper_child;
        if ((mapper_child = fork()) == -1) {
            perror("Failed to create mapper child");
            exit(1);
        }

        if (mapper_child == 0) {
            // is child
            // we do not need to write here
            if (close(mapper_pipes[i][1]) == -1) {
                perror("Failed to close read side of pipe for mapper");
                exit(1);
            }

            dup2(mapper_pipes[i][0], 0);

            ssize_t chars_read;
            char * buf;
            size_t size;
            while ((chars_read = getline(&buf, &size, stdin)) != -1) {
                fprintf(stderr, "MAPPER CHILD %zu: %s", i, buf);
            }

            exit(0);
        }
        mappers[i] = mapper_child;
    }

    for (i = 0; i < mapper_count; i++) {
        int splitter_status;
        waitpid(splitters[i], &splitter_status, 0);
        int mapper_status;
        waitpid(mappers[i], &mapper_status, 0);
    }
    // // Start a splitter process for each mapper.
    //     pid_t child;
    //     if ((child = fork()) == -1) {
    //         perror("Failed to create child\n");
    //     }

    //     if (child == 0) {
    //         // child process
            

    //         // Start all the mapper processes.
    //         exit(1);
    //     }
        

    // Start the reducer process.

    // Wait for the reducer to finish.

    // Print nonzero subprocess exit codes.

    // Count the number of lines in the output file.

    return 0;
}
