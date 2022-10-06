/**
 * deadlock_demolition
 * CS 341 - Fall 2022
 */
#include "libdrm.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void * first_funct(void * drm) {
    fprintf(stderr, "First Funct Start\n");
    pthread_t pid = pthread_self();
    int success = drm_wait(drm, &pid);
    assert(success == 1);
    sleep(5);
    fprintf(stderr, "Just woke up from a nap!\n");
    fprintf(stderr, "First Funct Finished\n");
    success = drm_post(drm, &pid);
    assert(success == 1);
    return NULL;
}

void * second_funct(void * drm) {
    fprintf(stderr, "Second Funct Start\n");
    pthread_t pid = pthread_self();
    int success = drm_wait(drm, &pid);
    assert(success == 1);
    fprintf(stderr, "After First Funct Ends\n");
    fprintf(stderr, "Second Funct Finished\n");
    success = drm_post(drm, &pid);
    assert(success == 1);
    return NULL;
}

int main() {
    drm_t *drm = drm_init();
    // TODO your tests here
    pthread_t first;
    pthread_t second;
    pthread_create(&first, NULL, first_funct, drm);
    pthread_create(&second, NULL, second_funct, drm);
    pthread_join(first, NULL);
    pthread_join(second, NULL);
    drm_destroy(drm);

    return 0;
}
