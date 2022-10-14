/**
 * password_cracker
 * CS 341 - Fall 2022
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"

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

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t main_barrier;

typedef struct _thread_job {
    char * hash;
    char * starting_str;
    char * ending_str;
    size_t offset;
} thread_job;

void _handle_signal(int sig) {
    fprintf(stderr, "Called here\n");
    exit(1);
}


void * t_crack_pass(void * arg) {
    pthread_mutex_lock(&lock);
        int tid = ++t_id;
    pthread_mutex_unlock(&lock);  

    thread_job job = (thread_job) arg;
    while (!FINISHED) { // might need to lock to avoid tsan error
        pthread_barrier_wait(&main_barrier); // Allow main thread to populate job data
        char * hash = job.hash;
        char * guess = job.starting_str;
        char * end_str = job.ending_str;
        size_t offset = job.offset;
        pthread_barrier_wait(&main_barrier); // Allow main thread to start making next job

        pthread_mutex_lock(&lock);
        v2_print_thread_start(tid, USERNAME, offset, guess);
        pthread_mutex_unlock(&lock);

        double start_time = getThreadCPUTime();
        // Attempt to find hash and collect data on hasing
        struct crypt_data cdata;
        cdata.initialized = 0;

        char * guess_hash = crypt_r(guess, "xx", &cdata);
        size_t hash_count = 1;
        while(strcmp(hash, guess_hash) != 0 && strcmp(guess, end_str) != 0) {
            pthread_mutex_lock(&lock);
            if (FOUND) {
                v2_print_thread_result(tid, hash_count, 1);
                break;
                pthread_mutex_unlock(&lock);
            }
            pthread_mutex_unlock(&lock);

            incrementString(guess);
            guess_hash = crypt_r(guess, "xx", &cdata);
            hash_count++;
        }
        // TODO: implement when thread did not find password in range
        // TODO: implement when thread found password in range
        double elapsed = getThreadCPUTime() - start_time;
        // TODO: remove this
        fprintf(stderr, "%f", elapsed);

        free(hash);
        free(guess);
        free(end_str);
    }   

    // FINISHED is true, return NULL
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

    // Inject NULL if CUR_LINE is null, set finished flag and have the threads return
    queue_push(job_queue, NULL);


    // Create thread pool
    pthread_t tids[thread_count];
    thread_job t_jobs[thread_count];
    for (i = 0; i < thread_count; i++) {
        pthread_create(tids + i, NULL, t_crack_pass, (void *)(t_jobs + i));
    }

    // Start each queue interation here, format information, then allow threads to crack
    pthread_barrier_init(&main_barrier, NULL, thread_count + 1);
    char * job;
    while((job = queue_pull(job_queue))) {
        // Populate CUR_LINE with a line from the queue
        FOUND = false;
        // Get username, hash, and hint
        char * job_addr = job;
        
        pthread_mutex_lock(&lock);
        USERNAME = strtok_r(line, " ", &line);
        v2_print_start_user(USERNAME);
        pthread_mutex_unlock(&lock);

        char * hash = strtok_r(NULL, " ", &line);
        char * starting_str = strtok_r(NULL, " ", &line);

        // Set up string to start guessing on
        int prefix_len = getPrefixLength(starting_str);
        memset(starting_str + prefix_len, 'a', strlen(starting_str) - prefix_len); // set periods to 'a'
        total_versions = (size_t)pow((double)26, (double)(strlen(starting_str) - prefix_len));

        assert(!strtok_r(NULL, " ", &line)); // Asserting no more arguments to parse

        // Calculate chunk sizes for each thread
        size_t chunk_size = total_versions / thread_count;
        size_t extra = total_versions % thread_count;

        size_t offset = 0;
        for (i = 0; i < thread_count; i++) {
            char * t_hash = strdup(hash);
            char * t_starting_str = strdup(starting_str);

            t_jobs[i].offset = offset;
            // get ending bound
            offset += chunk_size;
            if (extra) {
                offset += 1;
                extra--;
            }
            setStringPosition(starting_str, offset - 1);
            char * t_ending_str = strdup(starting_str);
            incrementString(starting_str);

            // set thread job info for all threads
            t_jobs[i].hash = t_hash;
            t_jobs[i].starting_str = t_starting_str;
            t_jobs[i].ending_str = t_ending_str;
        }

        pthread_barrier_wait(&main_barrier); // Let threads start cracking
        pthread_barrier_wait(&main_barrier); // Let threads read their job data
    }
    FINISHED = true;
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
