/**
 * vector
 * CS 341 - Fall 2022
 */
#include "vector.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    // Write your test cases here
    vector * test_vector_shallow = vector_create(NULL, NULL, NULL);

    // Test vector init values
    assert(test_vector_shallow);
    assert(vector_empty(test_vector_shallow));
    assert(vector_size(test_vector_shallow) == 0);
    assert(vector_capacity(test_vector_shallow) == 8);

    // resize then test
    vector_resize(test_vector_shallow, 14);
    assert(vector_size(test_vector_shallow) == 14);
    assert(vector_capacity(test_vector_shallow) == 16);

    // Test setting values (and getting values)
    int test_val = 42;
    vector_push_back(test_vector_shallow, &test_val);
    assert(*(int*)*vector_at(test_vector_shallow, vector_size(test_vector_shallow) - 1) == 42);

    test_val = 22;
    vector_set(test_vector_shallow, 0, &test_val);
    assert(*(int*)*vector_at(test_vector_shallow, 0) == 22);
    vector_destroy(test_vector_shallow);

    // Test vector reserve
    vector * int_vector = vector_create(&int_copy_constructor, &int_destructor, &int_default_constructor);
    vector_reserve(int_vector, 100);
    size_t i;
    for (i = 0; i < 100; i++) {
        int element_value = (int)(i + 1);
        vector_push_back(int_vector, &element_value);
    }
    for (i = 0; i < 100; i++) {
        int value = *(int *)*vector_at(int_vector, i);
        assert(value == (int)(i + 1));
    }

    // Test vector clear and vector insert
    vector_clear(int_vector);
    assert(vector_size(int_vector) == 0);
    int push_value = 1;
    vector_push_back(int_vector, &push_value);
    push_value = 2;
    vector_push_back(int_vector, &push_value);
    push_value = 3;
    vector_push_back(int_vector, &push_value);
    int insert_value = 5;
    vector_insert(int_vector, 1, &insert_value);
    assert(vector_size(int_vector) == 4);
    assert(*(int *)*vector_at(int_vector, 0) == 1);
    assert(*(int *)*vector_at(int_vector, 1) == 5);
    assert(*(int *)*vector_at(int_vector, 2) == 2);
    assert(*(int *)*vector_at(int_vector, 3) == 3);
    vector_clear(int_vector);
    vector_push_back(int_vector, &insert_value);
    vector_insert(int_vector, 0, &insert_value);
    vector_insert(int_vector, (vector_size(int_vector) - 1), &insert_value);

    // test vector erase
    vector_clear(int_vector);
    vector_push_back(int_vector, &push_value);
    vector_erase(int_vector, 0);
    assert(vector_size(int_vector) == 0);
    vector_destroy(int_vector);
    
    // test try to find error in test insert
    vector * string_vec = vector_create(&string_copy_constructor, &string_destructor, &string_default_constructor);
    char * push_string = "push back";
    vector_push_back(string_vec, push_string);

    char * insert_string = "insert this string!";
    for (i = 0; i < 16; i++) {
        vector_insert(string_vec, 0, insert_string);
        vector_insert(string_vec, 1, insert_string);
        vector_insert(string_vec, vector_size(string_vec) - 1, insert_string);
    }  
    assert(strcmp((char *)*vector_back(string_vec), push_string) == 0);
    for (i = 0; i < (16 - 1); i++) {
        assert(strcmp((char *)*vector_at(string_vec, i), insert_string) == 0);
    }
    vector_destroy(string_vec);
    return 0;
}
