/**
 * vector
 * CS 341 - Fall 2022
 */
#include "sstring.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Basic Creation Test
    sstring * basic_sstr = cstr_to_sstring("hello");
    char * basic_expected = "hello";
    char * basic_actual = sstring_to_cstr(basic_sstr);
    assert(strcmp(basic_expected, basic_actual) == 0);
    free(basic_actual);
    sstring_destroy(basic_sstr);

    // sstring_append Test
    sstring * base_sstr = cstr_to_sstring("Hello there! - ");
    sstring * to_append = cstr_to_sstring("General Kenobi...");
    sstring_append(base_sstr, to_append);
    char * append_expected = "Hello there! - General Kenobi...";
    char * append_actual = sstring_to_cstr(base_sstr);
    assert(strcmp(append_expected, append_actual) == 0);
    free(append_actual);
    sstring_destroy(base_sstr);
    sstring_destroy(to_append);

    // sstring_split Test
    sstring * to_split = cstr_to_sstring("Here,Are,Values,,");
    vector * split_expected = vector_create(&string_copy_constructor, &string_destructor, &string_default_constructor);
    vector_push_back(split_expected, "Here");
    vector_push_back(split_expected, "Are");
    vector_push_back(split_expected, "Values");
    vector_push_back(split_expected, "");
    vector_push_back(split_expected, "");
    vector * split_actual = sstring_split(to_split, ',');

    assert(vector_size(split_expected) == vector_size(split_actual));
    size_t i;
    for (i = 0; i < vector_size(split_expected); i++) {
        char * expected_split_str = (char *) *vector_at(split_expected, i);
        char * actual_split_str = (char *) *vector_at(split_actual, i);
        
        assert(strcmp(expected_split_str, actual_split_str) == 0);
    }
    sstring_destroy(to_split);
    vector_destroy(split_expected);
    vector_destroy(split_actual);

    // sstring_substitute Test
    sstring * sub_base = cstr_to_sstring("{} 1234");
    char * sub_expected = "Hello World!";
    assert(sstring_substitute(sub_base, 2, "1234", "World!") == 0);
    assert(sstring_substitute(sub_base, 0, "{}", "Hello") == 0);
    char * sub_actual = sstring_to_cstr(sub_base);
    assert(strcmp(sub_expected, sub_actual) == 0);
    free(sub_actual);
    sstring_destroy(sub_base);

    // sstring_slice Test
    sstring * slice_base = cstr_to_sstring("sliceKaelanme");
    char * slice_expected = "Kaelan";
    char * slice_actual = sstring_slice(slice_base, 5, 11);
    assert(strcmp(slice_expected, slice_actual) == 0);
    free(slice_actual);
    sstring_destroy(slice_base);
    
    return 0;
}
