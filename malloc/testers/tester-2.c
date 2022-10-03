/**
 * malloc
 * CS 341 - Fall 2022
 */
#include "tester-utils.h"

#include <stdio.h> // TODO: REMOVE THIS LINE
#define TOTAL_ALLOCS 2//5 * M

int main(int argc, char *argv[]) {
    malloc(1);
    fprintf(stderr, "Test 2: Allocated Sanity\n");

    int i;
    int **arr = malloc(TOTAL_ALLOCS * sizeof(int *));
    if (arr == NULL) {
        fprintf(stderr, "Memory failed to allocate!\n");
        return 1;
    }

    fprintf(stderr, "Test 2: Allocated 1\n");
    for (i = 0; i < TOTAL_ALLOCS; i++) {
        arr[i] = malloc(sizeof(int));
        if (arr[i] == NULL) {
            fprintf(stderr, "Memory failed to allocate!\n");
            return 1;
        }
        *(arr[i]) = i;
    }

    fprintf(stderr, "Test 2: Allocated 2\n");
    for (i = 0; i < TOTAL_ALLOCS; i++) {
        fprintf(stderr, "Iteration %d got: %d\n", i, *(arr[i]));
        if (*(arr[i]) != i) {
            fprintf(stderr, "Memory failed to contain correct data after many "
                            "allocations!\n");
            return 2;
        }
    }

    fprintf(stderr, "Test 2: Allocated 3\n");
    for (i = 0; i < TOTAL_ALLOCS; i++)
        free(arr[i]);

    free(arr);
    fprintf(stderr, "Memory was allocated, used, and freed!\n");
    return 0;
}
