/**
 * password_cracker
 * CS 341 - Fall 2022
 */
#include "cracker2.h"
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
#include <math.h>

static int t_id = 0;

static char * USERNAME;
static bool FINISHED;
static bool FOUND;
static char * PASSWORD;
static int TOTAL_HASHES;
static char * START_STR;
static char * HASH;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t main_barrier;

typedef struct _thread_job {
    long count;
    long offset;
    int prefix_len;
} thread_job;

void _handle_signal(int sig) {
    exit(1);
}


void * t_crack_pass(void * arg) {
    pthread_mutex_lock(&lock);
        int tid = ++t_id;
    pthread_mutex_unlock(&lock);  

    thread_job * job = (thread_job * ) arg;
    while (true) { 
        pthread_barrier_wait(&main_barrier); // Allow main thread to populate job data
        pthread_mutex_lock(&lock);
        if (FINISHED) {
            pthread_mutex_unlock(&lock);
            return NULL;
        }

        char * guess = strdup(START_STR);
        long count = job->count;
        long offset = job->offset;
        setStringPosition(guess + job->prefix_len, offset);

        v2_print_thread_start(tid, USERNAME, offset, guess);
        pthread_mutex_unlock(&lock);

        // Attempt to find hash and collect data on hasing
        struct crypt_data cdata;
        cdata.initialized = 0;

        
        size_t hash_count = 0;
        long i;
        bool self_found = false;
        char * guess_hash;
        for (i = 0; i < count; i++) {
            guess_hash = crypt_r(guess, "xx", &cdata);
            hash_count++;

            pthread_mutex_lock(&lock);
            if (FOUND) {
                pthread_mutex_unlock(&lock);
                break;
            } else if (strcmp(HASH, guess_hash) == 0) {
                FOUND = true;
                PASSWORD = strdup(guess);
                pthread_mutex_unlock(&lock);
                self_found = true;
                break;
            }
            pthread_mutex_unlock(&lock);

            incrementString(guess);
        }

        pthread_mutex_lock(&lock);
        if (self_found) {
            // this thread found the password
            v2_print_thread_result(tid, hash_count, 0);
        } else if (FOUND && (long)hash_count < count - 1) {
            // Not at the end of searching, we got halted
            v2_print_thread_result(tid, hash_count, 1);
        } else {
            v2_print_thread_result(tid, hash_count, 2);
        }
        TOTAL_HASHES += hash_count;
        pthread_mutex_unlock(&lock);
        free(guess);

        pthread_barrier_wait(&main_barrier);
    }   

    // FINISHED is true, return NULL
    return NULL;
}



int start(size_t thread_count) {
    // Remember to ONLY crack passwords in other threads
    pthread_barrier_init(&main_barrier, NULL, thread_count + 1);
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

    // Inject NULL if CUR_LINE is null, set finished flag and have the threads return
    queue_push(job_queue, NULL);


    // Create thread pool
    pthread_t tids[thread_count];
    thread_job t_jobs[thread_count];
    size_t i;
    for (i = 0; i < thread_count; i++) {
        pthread_create(tids + i, NULL, t_crack_pass, (void *)(t_jobs + i));
    }

    // Start each queue interation here, format information, then allow threads to crack
    char * job;
    while((job = queue_pull(job_queue))) {
        double start = getTime();
        double startcpu = getCPUTime();
        
        pthread_mutex_lock(&lock);
        TOTAL_HASHES = 0;
        FOUND = false;
        PASSWORD = NULL;
        USERNAME = NULL;
        pthread_mutex_unlock(&lock);
        // Get username, hash, and hint
        char * job_addr = job;
        
        pthread_mutex_lock(&lock);
        USERNAME = strtok_r(job, " ", &job);
        v2_print_start_user(USERNAME);
        

        HASH = strtok_r(NULL, " ", &job);
        START_STR = strtok_r(NULL, " ", &job);

        // Set up string to start guessing on
        int prefix_len = getPrefixLength(START_STR);
        int unknown_letter_count = strlen(START_STR) - prefix_len;
        memset(START_STR + prefix_len, 'a', unknown_letter_count); // set periods to 'a'
        pthread_mutex_unlock(&lock);
        // assert(!strtok_r(NULL, " ", &job)); // Asserting no more arguments to parse

        // Calculate chunk sizes for each thread
        for (i = 0; i < thread_count; i++) {
            long start_index = 0;
            long count = 0;

            getSubrange(unknown_letter_count, thread_count, i + 1, &start_index, &count);
            t_jobs[i].offset = start_index;
            t_jobs[i].count = count;
            t_jobs[i].prefix_len = prefix_len;
        }

        pthread_barrier_wait(&main_barrier); // Let threads start cracking
        pthread_barrier_wait(&main_barrier); // Wait for threads to get an answer

        double elapsed = getTime() - start;
        double elapsedcpu = getCPUTime() - startcpu;

        pthread_mutex_lock(&lock);
        if (FOUND) {
            v2_print_summary(USERNAME, PASSWORD, TOTAL_HASHES, elapsed, elapsedcpu, 0);
        } else {
            v2_print_summary(USERNAME, PASSWORD, TOTAL_HASHES, elapsed, elapsedcpu, 1);
        }
        
        free(job_addr);
        free(PASSWORD);
        pthread_mutex_unlock(&lock);
    }
    pthread_mutex_lock(&lock);
    FINISHED = true;
    pthread_mutex_unlock(&lock);
    pthread_barrier_wait(&main_barrier);

    // Join threads
    for (i = 0; i < thread_count; i++) {
        pthread_join(tids[i], NULL);
    }

    // Cleanup
    queue_destroy(job_queue);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
