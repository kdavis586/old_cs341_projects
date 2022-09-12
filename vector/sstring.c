/**
 * vector
 * CS 341 - Fall 2022
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char * internal_string = NULL;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    assert(input);
    sstring * new_sstring = malloc(sizeof(sstring));

    new_sstring->internal_string = malloc(strlen(input) + 1);
    strcpy(new_sstring->internal_string, input);

    return new_sstring;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    assert(input);

    char * cstr = malloc(strlen(input->internal_string) + 1);
    strcpy(cstr, input->internal_string);
    
    return cstr;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    assert(this);
    assert(addition);



    strcat(this->internal_string, addition->internal_string);

    return strlen(this->internal_string);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    assert(this);
    return NULL;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    assert(this);
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    assert(this);
    return NULL;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this);
}
