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

    char * input5 = "...";
    char * expected5[] = {"", "", "", NULL};

    char * input6 = "The Heisenbug is an incredible creature. Facenovel servers get their power from its indeterminism. Code smell can be ignored with INCREDIBLE use of air freshener. God objects are the new religion.";
    char * expected6[] = {"theHeisenbugIsAnIncredibleCreature",
                            "facenovelServersGetTheirPowerFromItsIndeterminism",
                            "codeSmellCanBeIgnoredWithIncredibleUseOfAirFreshener",
                            "godObjectsAreTheNewReligion",
                            NULL};

    char * input7 = "this is a test! USINGOTHER pUnCtUaTiOn341? should happen here,         ";
    char * expected7[] = {"thisIsATest", "usingotherPunctuation341", "shouldHappenHere", NULL};

    char * input8 = "big                            spaces                          are                    present.";
    char * expected8[] = {"bigSpacesArePresent", NULL};


    char ** actual1 = camelCaser(input1);
    char ** actual2 = camelCaser(input2);
    char ** actual3 = camelCaser(input3);
    char ** actual4 = camelCaser(input4);
    char ** actual5 = camelCaser(input5);
    char ** actual6 = camelCaser(input6);
    char ** actual7 = camelCaser(input7);
    char ** actual8 = camelCaser(input8);

    assert(compare_output(actual1, expected1));
    assert(compare_output(actual2, expected2));
    assert(compare_output(actual3, expected3));
    assert(compare_output(actual4, expected4));
    assert(compare_output(actual5, expected5));
    assert(compare_output(actual6, expected6));
    assert(compare_output(actual7, expected7));
    assert(compare_output(actual8, expected8));

    destroy(actual1);
    destroy(actual2);
    destroy(actual3);
    destroy(actual4);
    destroy(actual5);
    destroy(actual6);
    destroy(actual7);
    destroy(actual8);

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
