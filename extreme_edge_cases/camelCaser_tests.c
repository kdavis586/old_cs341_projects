/**
 * extreme_edge_cases
 * CS 241 - Fall 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int compare_output(char ** actual, char ** expected);
void print_camelcase(char ** to_print);

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    char * input1 = "Hello.World4 test.not here";
    char * expected1[] = {"hello", "world4Test", NULL};

    char * input2 = NULL;
    char ** expected2 = NULL;

    char * input3 = "";
    char * expected3[] = {NULL};

    char * input4 = "SOME STUFF HERE. AHHH AHHH..";
    char * expected4[] = {"someStuffHere", "ahhhAhhh", "", NULL};



    char ** actual1 = camelCaser(input1);
    char ** actual2 = camelCaser(input2);
    char ** actual3 = camelCaser(input3);
    char ** actual4 = camelCaser(input4);

    assert(compare_output(actual1, expected1));
    assert(compare_output(actual2, expected2));
    assert(compare_output(actual3, expected3));
    assert(compare_output(actual4, expected4));

    destroy(actual1);
    destroy(actual2);
    destroy(actual3);
    destroy(actual4);

    return 1;
}

int compare_output(char ** actual, char ** expected)  {
    if (!actual) {
        return actual == expected;
    }

    while (*actual && *expected) {
        int compare = strcmp(*actual, *expected);

        if (compare != 0) {
            return 0;
        }

        actual++;
        expected++;
    }

    return (*actual == NULL && *expected == NULL);
}

void print_camelcase(char ** to_print) {
    do {
        printf("%s\n", *to_print);
    } while (*to_print++);
    printf("\n");
}
