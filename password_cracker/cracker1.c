/**
 * password_cracker
 * CS 341 - Fall 2022
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#include <assert.h>
#include <crypt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int t_id = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void _handle_signal(int sig) {
    fprintf(stderr, "Called here\n");
    exit(1);
}


void * t_crack_pass(void * arg) {
    queue * job_queue = (queue *)arg;

    char * line = (char *)queue_pull(job_queue);
    char * line_start_addr = line;

    pthread_mutex_lock(&lock);
        int tid = ++t_id;
    pthread_mutex_unlock(&lock);

    while (line) {
        double start_time = getThreadCPUTime();

        // Split line to get the different arguments
        char * username = strtok_r(line, " ", &line);
        char * hash = strtok_r(NULL, " ", &line);
        char * hint = strtok_r(NULL, " ", &line);
        assert(!strtok_r(NULL, " ", &line)); // Asserting no more arguments to parse

        v1_print_thread_start(tid, username);

        // Set up string to start guessing on
        int prefix_len = getPrefixLength(hint);
        memset(hint + prefix_len, 'a', strlen(hint) - prefix_len); // set periods to 'a'
        char * guess = hint;

        // Set ending string to stop hashing if no match found
        char * end_str = strdup(hint);
        memset(end_str + prefix_len, 'z', strlen(end_str) - prefix_len);
        
        // Attempt to find hash and collect data on hasing
        struct crypt_data cdata;
        cdata.initialized = 0;

        char * guess_hash = crypt_r(guess, "xx", &cdata);
        size_t hash_count = 1;
        while(strcmp(hash, guess_hash) != 0 && strcmp(guess, end_str) != 0) {
            incrementString(guess);
            guess_hash = crypt_r(guess, "xx", &cdata);
            hash_count++;
        }

        double elapsed = getThreadCPUTime() - start_time;
        
        // Report results
        if ((strcmp(hash, guess_hash) == 0)) {
            v1_print_thread_result(tid, username, guess, hash_count, elapsed, 0);
        } else {
            v1_print_thread_result(tid, username, guess, hash_count, elapsed, 1);
        }

        free(end_str);
        free(line_start_addr);
        line = (char *)queue_pull(job_queue);
        line_start_addr = line;
    }   

    
    // At the end of the queue, RETURN
    return NULL;
}

int start(size_t thread_count) {
    // Remember to ONLY crack passwords in other threads
    signal(SIGQUIT, _handle_signal);

    queue * job_queue = queue_create(-1);
    ssize_t chars_read;
    char * buffer = NULL;
    size_t size = 0;

    // Read lines from file and add it to job queue
    while ((chars_read = getline(&buffer, &size, stdin)) != -1) {
        if (chars_read > 0) {
            char * line_dup = strdup(buffer);
            if (line_dup[strlen(buffer) - 1] == '\n') {
                line_dup[strlen(buffer) - 1] = '\0';
            }
            queue_push(job_queue, (void *)line_dup);
        }
        free(buffer);
        buffer = NULL;
    }
    free(buffer);

    // Inject NULL as many times as there are thread_count to help close threads
    // Idea: If thread pulls NULL from the queue, exit.
    // TODO: extend this to thread_count, currently testing this with only one thread
    size_t i;
    for (i = 0; i < thread_count; i++) {
        queue_push(job_queue, NULL);
    }


    // Create thread pool
    // TODO: extend this to thread_count, currently testing this with only one thread
    pthread_t tids[thread_count];
    for (i = 0; i < thread_count; i++) {
        pthread_create(tids + i, NULL, t_crack_pass, (void *) job_queue);
    }

    // Join threads
    // TODO: exted this to thread_count, currently testing this with only one thread
    for (i = 0; i < thread_count; i++) {
        pthread_join(tids[i], NULL);
    }

    // Cleanup
    queue_destroy(job_queue);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
