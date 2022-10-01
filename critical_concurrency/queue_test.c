/**
 * critical_concurrency
 * CS 341 - Fall 2022
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "queue.h"


void * no_bound_push(void * arg) {
    queue * q = (queue *) arg;
    size_t i;
    for (i = 0; i < 10; i++) {
        queue_push(q, NULL);
    }
    return NULL;
}

void * bound_push(void * arg) {
    queue * q = (queue *) arg;
    queue_push(q, NULL);
    return NULL;
}

void * bound_pull(void * arg) {
    queue * q = (queue *) arg;
    queue_pull(q);
    return NULL;
}

int main(int argc, char **argv) {
    // if (argc != 3) {
    //     printf("usage: %s test_number return_code\n", argv[0]);
    //     exit(1);
    // }
    
    // queue with no bound
    queue * q = queue_create(-1);
    pthread_t no_bound_tids[5];
    size_t i;
    for (i = 0; i < 5; i++) {
        pthread_create(no_bound_tids + i, NULL, no_bound_push, q);
    }
    for (i = 0; i < 5; i++) {
        pthread_join(no_bound_tids[i], NULL);
    }
    queue_destroy(q);
    free(q);
    // queue bound test
    q = queue_create(1);
    pthread_t bound_tids[4];
    for (i = 0; i < 4; i++) {
        void * (*func)(void*) = NULL;
        if (i % 2 == 0) {
            func = bound_pull;
        } else {
            func = bound_push;
        }

        pthread_create(bound_tids + i, NULL, func, q);
    }
    for (i = 0; i < 4; i++) {
        pthread_join(bound_tids[i], NULL);
    }
    queue_push(q, "hi");
    queue_destroy(q);
    free(q);
    return 0;
}
