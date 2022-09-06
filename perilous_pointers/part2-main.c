/**
 * perilous_pointers
 * CS 341 - Fall 2022
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // function 1
    first_step(81);

    // function 2
    int value2 = 132;
    second_step(&value2);

    // function 3
    int * value3 = (int *) malloc(sizeof(int));
    *value3 = 8942;
    double_step(&value3);
    free(value3);

    // function 4
    char * value4 = malloc(sizeof(int) * 6);
    *(int *)(value4 + 5) = 15;
    strange_step(value4);
    free(value4);

    // function 5
    char * value5 = "abc";
    empty_step(value5);

    // function 6
    char * value6 = "abcu";
    two_step(value6, value6);

    // function 7
    char * val7 = "abcdef";
    char * val8 = val7 + 2;
    char * val9 = val8 + 2;
    three_step(val7, val8, val9);

    // function 8
    char * val10 = "aa";
    char * val11 = malloc(3);
    val11[2] = val10[1] + 8;
    char * val12 = malloc(4);
    val12[3] = val11[2] + 8;
    step_step_step(val10, val11, val12);
    free(val11);
    free(val12);

    // function 9
    char * val13 = "\x01";
    int b = 1;
    it_may_be_odd(val13, b);

    // function 10
    char * to_copy = "test,CS241";
    char * val14 = malloc(strlen(to_copy) + 1);
    strcpy(val14, to_copy);
    tok_step(val14);
    free(val14);

    // function 11
    char * val15 = malloc(sizeof(int));
    val15[0] = 1;
    the_end(val15, val15);
    free(val15);

    return 0;
}
