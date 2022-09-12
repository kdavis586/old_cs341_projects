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

/* Forward declaration of helper functions */
bool _found_target(char * cur, char * target);
void _replace(sstring * this, char ** cur, char * target, char * substitution);
void _add_substr(vector * vec, char * cstr, size_t start, size_t end);

/* Structure definition */
struct sstring {
    // Anything you want
    char * internal_string;
};

/* Main Functions */
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
        strlen(this->internal_string) + strlen(addition->internal_string) + 1, 
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
    size_t substr_start = 0;
    for (i = 0; i < strlen(cstr); i++) {
        if (cstr[i] == delimiter) {
            _add_substr(split_res, cstr, substr_start, i);
            substr_start = i + 1;
        }
    }

    // add final split
    _add_substr(split_res, cstr, substr_start, i);

    return split_res;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    assert(this);
    assert(target);
    assert(substitution);

    char * cstr = this->internal_string;
    char * cur = &cstr[offset];

    bool matched = false;
    while (*cur) {
        if (*cur == target[0]) {
            // potential replacement, see if so
            if (_found_target(cur, target)) {
                _replace(this, &cur, target, substitution);
                matched = true;
                continue;
            }
        }
        cur++;
    }

    return (matched) ? 0 : -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    assert(this);
    assert(0 <= start);
    assert(start <= end);
    assert((size_t)end <= strlen(this->internal_string));

    char * slice = calloc(end - start + 1, sizeof(char));
    strncpy(slice, this->internal_string + start, end - start);

    return slice;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this);
    free(this->internal_string);
    this->internal_string = NULL;
    free(this);
    this = NULL;
}

/* Helper Functions */
bool _found_target(char * cur, char * target) {
    char * cur_itr = cur;
    char * target_itr = target;

    while (*cur_itr && *target_itr) {
        if (*cur_itr != *target_itr) {
            return false;
        }

        cur_itr++;
        target_itr++;
    }

    return true;
}

void _replace(sstring * this, char ** cur, char * target, char * substitution) {
    size_t front_len = *cur - this->internal_string;
    char * front = calloc(front_len + 1, sizeof(char));
    strncpy(front, this->internal_string, front_len);

    size_t whole_len = strlen(this->internal_string);
    size_t back_len = &this->internal_string[whole_len] - (*cur + strlen(target));
    char * back = calloc(back_len + 1, sizeof(char));
    strcpy(back, *cur + strlen(target));

    char * replace_str = calloc(strlen(front) + strlen(back) + strlen(substitution) + 1, sizeof(char));
    strcpy(replace_str, front);
    strcat(replace_str, substitution);
    strcat(replace_str, back);

    free(this->internal_string);
    this->internal_string = replace_str;
    *cur = &(this->internal_string[strlen(front) + strlen(substitution)]);

    free(front);
    free(back);
}

void _add_substr(vector * vec, char * cstr, size_t start, size_t end) {
    size_t substr_len = end - start;
    char * split_str = calloc(substr_len + 1, sizeof(char));
    strncpy(split_str, &cstr[start], substr_len);
    vector_push_back(vec, split_str);
    free(split_str); // copy constructor made a deep copy, no longer need this memory
}
