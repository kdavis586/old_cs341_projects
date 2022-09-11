/**
 * vector
 * CS 341 - Fall 2022
 */
#include "vector.h"

#include <assert.h>

int main(int argc, char *argv[]) {
    // Write your test cases here
    vector * test_vector_shallow = vector_create(NULL, NULL, NULL);

    // Test vector init values
    assert(test_vector_shallow);
    assert(vector_size(test_vector_shallow) == 0);
    assert(vector_capacity(test_vector_shallow) == 8);
    vector_destroy(test_vector_shallow);

    return 0;
}
