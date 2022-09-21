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
    void *p3 = malloc(50); 
    p2 = realloc(p2, 10);  
    p1 = realloc(p1, 100);
    free(p2); // should free 10
    free(p1); // should free 100
    p3 = realloc(p3, 0); //should free 50
    int * int_arr = calloc(3, sizeof(int));
    assert(!int_arr[1]);
    free(int_arr);
    free(int_arr);
    return 0;
}