/**
 * teaching_threads
 * CS 341 - Fall 2022
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct _thread_task {
    int * list;
    reducer reduce_func;
    size_t section_len;
    size_t start;
    int base_case;
} thread_task;


/* You should create a start routine for your threads. */
void * thread_reduce(void * arg) {
    thread_task * tt = (thread_task *) arg;
    size_t end = tt->start + tt->section_len;

    int * result = (int *) malloc(sizeof(int));
    *result = tt->base_case;
    size_t i;
    for (i = tt->start; i < end; i++) {
        *result = tt->reduce_func(*result, tt->list[i]);
    }

    return (void *) result;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    int result = base_case;
    
    size_t needed_threads = num_threads;
    if (list_len < num_threads) {
        needed_threads = list_len;
    }

    pthread_t tids[needed_threads];
    size_t assign_start = 0;
    size_t base_assign = list_len / needed_threads; // Definitely how many elements each thread gets
    size_t extra = list_len % needed_threads; // len is not divisible by num threads, assign extra to only some threads

    thread_task * tt_arr[needed_threads];
    size_t i;
    for (i = 0; i < needed_threads; i++) {  
        thread_task * tt = malloc(sizeof(thread_task));
        tt->list = list;
        tt->reduce_func = reduce_func;
        tt->section_len = (extra) ? base_assign + 1 : base_assign; // Assign first few threads with an extra length
        tt->start = assign_start; // Make sure each thread is working withing a specific offest of the list
        tt->base_case = base_case;
        tt_arr[i] = tt;

        if (extra) {
            extra--;
        }

        assign_start += tt->section_len; // Change the starting point for the next thread to avoid overlap
        pthread_create(tids + i, NULL, thread_reduce, tt);
    } 

    int out_arr[needed_threads];
    for (i = 0; i < needed_threads; i++) {
        void * val;
        pthread_join(tids[i], &val);
        free(tt_arr[i]);
        
        out_arr[i] = * (int *) val;
        free(val);
    }

    // Final reduction
    for (i = 0; i < needed_threads; i++) {
        result = reduce_func(result, out_arr[i]);
    }

    return result;
}
