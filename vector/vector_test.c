/**
 * vector
 * CS 341 - Fall 2022
 */
#include "vector.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Write your test cases here
    vector * test_vector_shallow = vector_create(NULL, NULL, NULL);

    // Test vector init values
    assert(test_vector_shallow);
    assert(vector_empty(test_vector_shallow));
    assert(vector_size(test_vector_shallow) == 0);
    assert(vector_capacity(test_vector_shallow) == 8);

    // resize then test
    vector_resize(test_vector_shallow, 14);
    assert(vector_capacity(test_vector_shallow) == 16);

    // Test setting values (and getting values)
    int test_val = 42;
    vector_push_back(test_vector_shallow, &test_val);
    assert(*(int*)*vector_at(test_vector_shallow, 0) == 42);

    test_val = 22;
    vector_set(test_vector_shallow, 0, &test_val);
    assert(*(int*)*vector_at(test_vector_shallow, 0) == 22);
    vector_destroy(test_vector_shallow);

    return 0;
}
