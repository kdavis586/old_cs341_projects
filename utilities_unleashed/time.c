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
#include <stdio.h>

#define NANO_IN_SEC (double)(1000000000)
extern char** environ;

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        print_time_usage();
        exit(1);
    }

    struct timespec start, end;
    int sst = clock_gettime(CLOCK_MONOTONIC, &start);
    if (sst == -1) {
        // Handling if getting clock time failed
        exit(1);
    }

    pid_t child = fork();
    // Handle if forking failed
    if (child == -1) {
        print_fork_failed();
    }

    if (child == 0) {
        execvp(argv[1], &argv[1]);
        exit(1);
    } else {
        int status;
        pid_t pid = waitpid(child, &status, 0);

        // Handle if child process did not return properly
        if (pid == -1) {
            print_exec_failed();
        }

        // Handle if child failed.
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            print_exec_failed();
            exit(1);
        }

        int se = clock_gettime(CLOCK_MONOTONIC, &end);
        if (se == -1) {
            // Handling if getting clock time failed
            exit(1);
        }

        double duration_s = (double)(end.tv_sec - start.tv_sec) + ((end.tv_nsec / NANO_IN_SEC) - (start.tv_nsec / NANO_IN_SEC));
        display_results(argv, duration_s);
        printf("Parent is done\n");
    }

    exit(0);
}
