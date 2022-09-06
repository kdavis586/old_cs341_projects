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
    printf("%s\n", first_step(81));

    // function 2
    int value = 132
    printf("%s\n", second_step(&value));

    // function 3
    int ** value = {8942};
    printf("%s\n", double_step(value));

    // function 4
    char * value = "abcde\15";
    printf("%s\n", strange_step(value));

    // function 5
    char * value = "abc";
    printf("%s\n", empty_step(value));

    // function 6
    char * value = "abcu";
    printf("%s\n", two_step(value, value));

    // function 7
    char * val1 = "abcdef";
    char * val2 = val1 + 2;
    char * val3 = val2 + 2;
    printf("%s\n", three_step(val1, val2, val3));

    // function 8
    printf("%s\n", first_step(81));

    // function 9
    printf("%s\n", first_step(81));

    // function 10
    printf("%s\n", first_step(81));

    // function 11
    printf("%s\n", first_step(81));

    return 0;
}
