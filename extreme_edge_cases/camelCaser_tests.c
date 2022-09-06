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
    // TODO: Implement me!
    char * input1 = "Hello.World4 test.not here";
    char * expected1[] = {"hello", "world4Test", NULL};

    char ** actual1 = camelCaser(input1);
    print_camelcase(actual1);
    print_camelcase(expected1);
    assert(compare_output(actual1, expected1));
    destroy(actual1);

    return 1;
}

int compare_output(char ** actual, char ** expected)  {
    while (*actual && *expected) {
        int compare = strcmp(*actual, *expected);

        if (!compare) {
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
