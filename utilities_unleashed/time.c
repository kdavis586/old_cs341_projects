/**
 * utilities_unleashed
 * CS 341 - Fall 2022
 */

#include "format.h"

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    struct timespec * start_time = malloc(sizeof(struct timespec));
    int sst = clock_gettime(CLOCK_MONOTONIC, start_time);

    if (sst == -1) {
        return 1;
    }

    double start_nano = start_time->tv_nsec;
    free(start_time);

    pid_t pid = fork();

    if (pid == -1) {
        print_fork_failed();
    }

    char ** terminated_argv = (char**) malloc(argc + 1); // This might cause errors
    memcpy(terminated_argv, argv, argc);
    terminated_argv[sizeof(terminated_argv) - 1] = NULL;

    if (pid == 0) {
        // Child process
        // execute something here
        // add null pointer to end of array
        execv(terminated_argv[0], terminated_argv);
        print_exec_failed();
        return 1;
    }
    
    int status = 0;
    wait(&status);
    free(argv); // might not need this...

    struct timespec * end_time = malloc(sizeof(struct timespec));
    int se = clock_gettime(CLOCK_MONOTONIC, end_time);

    if (se == -1) {
        return 1;
    }

    double end_nano = end_time->tv_nsec;
    free(end_time);

    double duration_s = (end_nano - start_nano) / 1000000000;
    display_results(argv, duration_s);
    return 0;
}
