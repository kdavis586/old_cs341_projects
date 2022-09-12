/**
 * vector
 * CS 341 - Fall 2022
 */
#include "vector.h"

#include <assert.h>
#include <stdio.h>

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

    // test vector erase
    vector_erase(int_vector, 1);
    assert(vector_size(int_vector) == 3);
    assert(*(int *)*vector_at(int_vector, 0) == 1);
    assert(*(int *)*vector_at(int_vector, 1) == 2);
    assert(*(int *)*vector_at(int_vector, 2) == 3);
    vector_destroy(int_vector);

    // remaking the vector to reset capacity;
    int_vector = vector_create(&int_copy_constructor, &int_destructor, &int_default_constructor);

    // Test push back and vector intsert trigger automatic reallocation
    int value = 1000;
    for (i = 0; i < 255; i++) {
        vector_push_back(int_vector, &value);
    }
    for (i = 0; i < 255; i++) {
        assert(*(int *)*vector_at(int_vector, i) == value);
    }

    vector_destroy(int_vector);
    int_vector = vector_create(NULL, NULL, NULL);
    int last_value = 343;
    vector_push_back(int_vector, &last_value);
    for (i = 0; i < 255; i++) {
        vector_insert(int_vector, 0, &value);
    }
    for (i = 0; i < vector_size(int_vector) - 1; i++) {
        assert(*(int *)*vector_at(int_vector, i) == value);
    }
    assert(*(int *)*vector_back(int_vector) == last_value);
    vector_erase(int_vector, 0);
    vector_clear(int_vector);
    vector_destroy(int_vector);
    
    return 0;
}
