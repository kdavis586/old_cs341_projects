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
    semm_wait(&s1);
    
    return NULL;
}

void * pass_post(void * arg) {
    semm_post(&s1);
    
    return NULL;
}

int main(int argc, char **argv) {
    semm_init(&s1, 2, 2);

    printf("Testing block on wait and post\n--------------------------------\n");
    pthread_t tids[9];
    size_t i;
    for (i = 0; i < 9; i++) {
        if (i < 3 || i > 6) {
            pthread_create(tids + i, NULL, pass_wait, tids + i);
        } else {
            pthread_create(tids + i, NULL, pass_post, tids + i);
        }

        sleep(1);
    }

    for (i = 0; i < 8; i++) {
        pthread_join(tids[i], NULL);
    }
    
    semm_destroy(&s1);
    printf("Finished!\n");
    return 0;
}
