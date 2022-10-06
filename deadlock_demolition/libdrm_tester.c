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

void * test1_funct1(void * drm) {
    fprintf(stderr, "First Funct Start\n");
    pthread_t tid = pthread_self();
    int success = drm_wait(drm, &tid);
    assert(success == 1);
    sleep(5);
    fprintf(stderr, "Just woke up from a nap!\n");
    fprintf(stderr, "First Funct Finished\n");
    success = drm_post(drm, &tid);
    assert(success == 1);
    return NULL;
}

void * test1_funct2(void * drm) {
    fprintf(stderr, "Second Funct Start\n");
    pthread_t tid = pthread_self();
    int success = drm_wait(drm, &tid);
    assert(success == 1);
    fprintf(stderr, "After First Funct Ends\n");
    fprintf(stderr, "Second Funct Finished\n");
    success = drm_post(drm, &tid);
    assert(success == 1);
    return NULL;
}

int main() {
    drm_t *drm = drm_init();

    // Simple Mutex Lock Test
    fprintf(stderr, "---------------- Test 1 ----------------\n");
    pthread_t first;
    pthread_t second;
    pthread_create(&first, NULL, test1_funct1, drm);
    pthread_create(&second, NULL, test1_funct2, drm);
    pthread_join(first, NULL);
    pthread_join(second, NULL);
    fprintf(stderr, "\n\n");

    // Simple Deadlock Test
    fprintf(stderr, "---------------- Test 2 ----------------\n");
    pthread_t main_tid = pthread_self();
    int success = drm_wait(drm, &main_tid);
    assert(success == 1);
    success = drm_wait(drm, &main_tid);
    assert(success == 0);
    success = drm_post(drm, &main_tid);
    assert(success == 1);
    success = drm_post(drm, &main_tid);
    assert(success == 0);
    fprintf(stderr, "\n\n");
    drm_destroy(drm);

    // Multi-drm Deadlock Test
    fprintf(stderr, "---------------- Test 3 ----------------\n");
    drm_t * a = drm_init();
    drm_t * b = drm_init();
    drm_t * c = drm_init();
    pthread_t p1;
    pthread_t p2;
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, 2);
    // TODO: Recreate Q4 on dealock worksheet with only p1 and p2 and only a, b, c

    fprintf(stderr, "--------------- Finished ---------------\n");
    return 0;
}
