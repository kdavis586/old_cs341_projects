/**
 * deadlock_demolition
 * CS 341 - Fall 2022
 */
#include "libdrm.h"
#include "vector.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Thread functions for test 1
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


// Thread functions/struct/struct functions for test 3
typedef struct _test3struct {
    pthread_barrier_t * barrier;
    vector * drms;
} test3struct;

void * test3_funct1(void * arg) {
    test3struct * info = (test3struct *) arg;
    vector * drms = info->drms;
    drm_t * a = vector_get(drms, 0);
    drm_t * b = vector_get(drms, 1);
    drm_t * c = vector_get(drms, 2);
    pthread_t tid = pthread_self();

    int a_success = drm_wait(a, &tid);
    int b_success = drm_wait(b, &tid);
    assert(a_success == 1);
    assert(b_success == 1);
    pthread_barrier_wait(info->barrier);
    fprintf(stderr, "Test 3 Function 1 Pass barrier 1\n");
    int c_success = drm_wait(c, &tid);
    assert(c_success == 0);

    a_success = drm_post(a, &tid);
    b_success = drm_post(b, &tid);
    assert(a_success == 1);
    assert(b_success == 1);

    return NULL;
}

void * test3_funct2(void * arg) {
    test3struct * info = (test3struct *) arg;
    vector * drms = info->drms;
    drm_t * b = vector_get(drms, 1);
    drm_t * c = vector_get(drms, 2);
    pthread_t tid = pthread_self();
    pthread_barrier_wait(info->barrier);

    fprintf(stderr, "Test 3 Function 2 Pass barrier 1\n");
    int c_success = drm_wait(c, &tid);
    int b_success = drm_wait(b, &tid);
    assert(c_success == 1);
    assert(b_success == 1);

    c_success = drm_post(c, &tid);
    b_success = drm_post(b, &tid);
    assert(c_success == 1);
    assert(b_success == 1);

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
    fprintf(stderr, "\n");

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
    fprintf(stderr, "\n");
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

    vector * drms = shallow_vector_create();
    vector_push_back(drms, a);
    vector_push_back(drms, b);
    vector_push_back(drms, c);

    test3struct info;
    info.barrier = &barrier;
    info.drms = drms;

    pthread_create(&p1, NULL, test3_funct1, &info);
    pthread_create(&p2, NULL, test3_funct2, &info);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);

    pthread_barrier_destroy(&barrier);
    vector_destroy(drms);
    fprintf(stderr, "\n");

    fprintf(stderr, "--------------- Finished ---------------\n");
    return 0;
}
