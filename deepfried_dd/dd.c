/**
 * deepfried_dd
 * CS 341 - Fall 2022
 */
#include "format.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static FILE * INPUT;
static FILE * OUTPUT;
static size_t TOTAL_BYTES_COPIED = 0;
static size_t FULL_BLOCKS_IN = 0;
static size_t FULL_BLOCKS_OUT = 0;
static size_t PARTIAL_BLOCKS_IN = 0;
static size_t PARTIAL_BLOCKS_OUT = 0;
static struct timespec START;

void handler(int signal) {
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);

    time_t elapsed_sec = end.tv_sec - START.tv_sec;
    long elapsed_nsec = end.tv_nsec - START.tv_nsec;
    double elapsed = ((double)(elapsed_sec * 1000000000) + (double)elapsed_nsec) / (double)1000000000;
    print_status_report(FULL_BLOCKS_IN, PARTIAL_BLOCKS_IN,
        FULL_BLOCKS_OUT, PARTIAL_BLOCKS_OUT, TOTAL_BYTES_COPIED, elapsed);
}

void handle_exit(void) {
    if (INPUT && INPUT != stdin) {
        fclose(INPUT);
        //free(INPUT);
    }

    if (OUTPUT && OUTPUT != stdout) {
        fclose(OUTPUT);
        //free(OUTPUT);
    }
}

int main(int argc, char **argv) {
    INPUT = stdin;
    OUTPUT = stdout;
    int c;
    size_t block_size = 512;
    ssize_t total_block_cpy = -1;
    size_t skip_block_in = 0;
    size_t skip_block_out = 0;

    signal(SIGUSR1, handler);
    // Parse options
    while ((c = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch (c) {
            case 'i':
                INPUT = fopen(optarg, "r");
                if (!INPUT) {
                    print_invalid_input(optarg);
                    handle_exit();
                    exit(1);
                }
                break;
            case 'o':
                OUTPUT = fopen(optarg, "w+");
                if (!OUTPUT) {
                    print_invalid_output(optarg);
                    handle_exit();
                    exit(1);
                }
                break;
            case 'b':
                if (sscanf(optarg, "%zu", &block_size) != 1) {
                    handle_exit();
                    exit(1);
                }
                break;
            case 'c':
                if (sscanf(optarg, "%zd", &total_block_cpy) != 1) {
                    handle_exit();
                    exit(1);
                }
                break;
            case 'p':
                if (sscanf(optarg, "%zu", &skip_block_in) != 1) {
                    handle_exit();
                    exit(1);
                }
                break;
            case 'k':
                if (sscanf(optarg, "%zu", &skip_block_out) != 1) {
                    handle_exit();
                    exit(1);
                }
                break;
            default:
                handle_exit();
                exit(1);
        }
    }

    if (clock_gettime(CLOCK_REALTIME, &START) == -1) {
        handle_exit();
        exit(1);
    }

    if (INPUT != stdin && fseek(INPUT, skip_block_in * block_size, SEEK_SET) == -1) {
        handle_exit();
        exit(1);
    }
    if (OUTPUT != stdout && fseek(OUTPUT, skip_block_out * block_size, SEEK_SET) == -1) {
        handle_exit();
        exit(1);
    }

    // getting the end position of the file
    size_t cur_pos = (size_t)ftell(INPUT);
    fseek(INPUT, 0, SEEK_END);
    size_t end_pos = (size_t)ftell(INPUT);
    fseek(INPUT, cur_pos, SEEK_SET);
    
    while (end_pos - cur_pos >= block_size && (total_block_cpy == -1 || (ssize_t)FULL_BLOCKS_IN < total_block_cpy)) {
        char buf[block_size];
        memset(&buf, 0, block_size);
        if (fread(&buf, block_size, 1, INPUT) == 1) {
            FULL_BLOCKS_IN += 1;
            if (fwrite(&buf, block_size, 1, OUTPUT) != 1) {
                exit(1);
            }
            FULL_BLOCKS_OUT += 1;
            TOTAL_BYTES_COPIED += block_size;
            cur_pos = (size_t)ftell(INPUT);
        } else {
            exit(1);
        }
    }
    if (end_pos - cur_pos && end_pos - cur_pos < block_size) {
        // could still be data to read
        size_t size = end_pos - cur_pos;
        char buf[size];
        memset(&buf, 0, size);
        if (fread(&buf, size, 1, INPUT) == 1) {
            PARTIAL_BLOCKS_IN += 1;
            if (fwrite(&buf, strlen(buf), 1, OUTPUT) != 1) {
                exit(1);
            }

            PARTIAL_BLOCKS_OUT += 1;
            TOTAL_BYTES_COPIED += size;
        }
    }
    // if (feof(INPUT)) {
    //     if (strlen(buf)) {
    //         PARTIAL_BLOCKS_IN += 1;
    //         if (fwrite(&buf, strlen(buf), 1, OUTPUT) != 1) {
    //             exit(1);
    //         }

    //         PARTIAL_BLOCKS_OUT += 1;
    //         TOTAL_BYTES_COPIED += (strlen(buf));
    //     } 
    // } else
    if (ferror(INPUT)) {
        exit(1);
    }
    

    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);

    time_t elapsed_sec = end.tv_sec - START.tv_sec;
    long elapsed_nsec = end.tv_nsec - START.tv_nsec;
    double elapsed = ((double)(elapsed_sec * 1000000000) + (double)elapsed_nsec) / (double)1000000000;

    
    print_status_report(FULL_BLOCKS_IN, PARTIAL_BLOCKS_IN,
        FULL_BLOCKS_OUT, PARTIAL_BLOCKS_OUT, TOTAL_BYTES_COPIED, elapsed);
    handle_exit();
    return 0;
}