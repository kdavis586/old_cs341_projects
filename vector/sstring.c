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
    char * internal_string = calloc(1, sizeof(char));
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

    this->internal_string = reallocarray(this->internal_string,
        strlen(this->internal_string) + strnlen(addition->internal_string) + 1,
        sizeof(char));

    strcat(this->internal_string, addition->internal_string);

    return strlen(this->internal_string);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    assert(this);
    char * cstr = this->internal_string;
    vector * split_res = vector_create(&string_copy_constructor, &string_destructor, &string_default_constructor);
    size_t i;
    char * substr_start = 0;
    for (i = 0; i < strlen(cstr); i++) {
        if (cstr[i] == delimiter) {
            size_t substr_len = i - substr_start - 1;
            char * split_str = calloc(substr_len + 1, sizeof(char));
            strncpy(split_str, &cstr[substr_start], substr_len);
            vector_push_back(split_res, split_str);
            free(split_str); // copy constructor made a deep copy, no longer need this memory
        }
    }

    return split_res;
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
    assert(0 <= start);
    assert(start <= end);
    assert(end <= strlen(this->internal_string));

    char * slice = calloc(end - start + 1, sizeof(char));
    strncpy(slice, this->internal_string + start, end - start);

    return slice;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this);
    free(this->internal_string);
    this->internal_string = NULL;
    free(this)
    this = NULL;
}
