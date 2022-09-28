/**
 * critical_concurrency
 * CS 341 - Fall 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "semamore.h"
static Semamore s1;

void * pass_wait(void * arg) {
    printf("%s is in pass function...\n", (char *) arg);
    semm_wait(&s1);
    printf("%s got past\n", (char *) arg);
    
    return NULL;
}

void * pass_post(void * arg) {
    printf("%s is in pass function...\n", (char *) arg);
    semm_post(&s1);
    printf("%s got past\n", (char *) arg);
    
    return NULL;
}

int main(int argc, char **argv) {
    semm_init(&s1, 2, 4);

    // Create some threads

    // Test block on wait
    printf("Testing block on wait\n");
    pthread_t tids[4];
    pthread_create(tids, NULL, pass_wait, "Thread 1");
    pthread_create(tids + 1, NULL, pass_wait, "Thread 2");
    pthread_create(tids + 2, NULL, pass_wait, "Thread 3");
    pthread_create(tids + 3, NULL, pass_wait, "Thread 4");

    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);
    printf("Only 2 threads here\n");
    semm_post(&s1);
    semm_post(&s1);
    pthread_join(tids[2], NULL);
    pthread_join(tids[3], NULL);
    printf("\n\n");
    // Test block on post
    printf("Testing block on post\n");
    pthread_create(tids, NULL, pass_post, "Thread 1");
    pthread_create(tids + 1, NULL, pass_post, "Thread 2");
    pthread_create(tids + 2, NULL, pass_post, "Thread 3");
    pthread_create(tids + 3, NULL, pass_post, "Thread 4");

    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);
    printf("Only 2 threads here\n");
    semm_wait(&s1);
    semm_wait(&s1);
    pthread_join(tids[2], NULL);
    pthread_join(tids[3], NULL);
    
    semm_destroy(&s1);
    return 0;
}
