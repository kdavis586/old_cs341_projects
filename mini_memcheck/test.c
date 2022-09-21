/**
 * mini_memcheck
 * CS 341 - Fall 2022
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main() {
    void *p1 = malloc(30); 
    void *p2 = malloc(40); 
    void *p3 = malloc(50); //req = 120
    p2 = realloc(p2, 10);  // free = 30
    p1 = realloc(p1, 100); // req = 190
    void *temp = realloc(p1, 1000000000000000);
    if (temp) {
        p1 = temp;
    }
    free(p2); // free = 40
    free(p1); // free = 140
    p3 = realloc(p3, 0); // free = 190
    int * int_arr = calloc(3, sizeof(int)); // req = 202
    assert(!int_arr[1]);
    free(int_arr); // free = 202
    free(int_arr);
    return 0;
}