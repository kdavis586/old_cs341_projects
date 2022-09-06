/**
 * extreme_edge_cases
 * CS 241 - Fall 2022
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

char * camel_case_sentence(const char * sentence);
size_t get_sentence_count(const char * src);
void copy_chars(char * dest, const char * src, size_t length);
void handle_punct(size_t idx, char ** output, const char * input_str, const char * word_start);
size_t get_bad_char_count(const char * src);
int skip_char(char src);

char **camel_caser(const char * input_str) {
    //If input is null return null
    if (!input_str) {
        return NULL;
    }

    char ** output = NULL;
    const char * itr = input_str;
    size_t count = get_sentence_count(itr);

    output = (char **) malloc((count + 1) * sizeof(char **));
    output[count] = NULL;

    const char * word_start = input_str;

    size_t idx = 0;
    while (*input_str) {
        if (ispunct(*input_str)) {
            handle_punct(idx, output, input_str, word_start);
            idx++;
            word_start = (input_str + 1);
        }
        input_str++;
    }

    return output;
}

void destroy(char **result) {
    if (!result) {
        return;
    }
    
    char ** inc = result;

    while (*inc) {
        char * temp = *inc;

        if (temp) {
            free(temp);
            temp = NULL;
        }
        
        inc++;
    }

    if (result) {
        free(result);
        result = NULL;
    }
}

void handle_punct(size_t idx, char ** output, const char * input_str, const char * word_start) {
    size_t length = input_str - word_start;
    char * camel_cased = NULL;

    if (length) {
        char * sentence = (char *) malloc(length + 1);

        if (!sentence) {
            perror("malloc failed!");
            exit(1);
        }

        sentence[length] = '\0';
        copy_chars(sentence, word_start, length);

        camel_cased = camel_case_sentence(sentence);

        free(sentence);
    } else {
        camel_cased = "";
    }

    output[idx] = camel_cased;
}

size_t get_sentence_count(const char * src) {
    size_t count = 0;
    
    while(*src) {
        if (ispunct(*src)) {
            count++;
        }

        src++;
    }

    return count;
}

void copy_chars(char * dest, const char * src, size_t length) {
    size_t i = 0;

    for ( ; i < length; i++) {
        dest[i] = src[i];
    }
}

char * camel_case_sentence(const char * sentence) {
    if (!sentence) {
        return "";
    }

    const char * itr = sentence;
    size_t bad_chars = get_bad_char_count(itr);

    char * camel_cased = (char *) malloc(strlen(sentence) - bad_chars + 1);
    camel_cased[strlen(sentence)] = '\0';

    size_t idx = 0;
    int first_letter = 1;
    int first_word = 1;
    while(*sentence) {
        int skip = skip_char(*sentence);

        if (!skip) {
            if (first_letter && first_word) {
                camel_cased[idx] = tolower(*sentence);
                first_letter = 0;
                first_word = 0;
            } else if (first_letter) {
                camel_cased[idx] = toupper(*sentence);
                first_letter = 0;
            } else {
                camel_cased[idx] = tolower(*sentence);
            }

            idx++;
        } else if (!first_letter) {
            first_letter = 1;
        }

        sentence++;
    }
    //strcpy(camel_cased, sentence);

    return camel_cased;
}

size_t get_bad_char_count(const char * src) {
    size_t count = 0;
    
    while(*src) {
        if(skip_char(*src)) {
            count++;
        }

        src++;
    }

    return count;
}

int skip_char(char src) {
    return (ispunct(src) || isspace(src));
}