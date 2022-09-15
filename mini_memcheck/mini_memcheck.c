/**
 * mini_memcheck
 * CS 341 - Fall 2022
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    total_memory_requested += request_size;

    meta_data * node = malloc(sizeof(meta_data) + request_size);
    node->request_size = request_size;
    node->filename = filename;
    node->instruction = instruction;
    node->next = head;

    head = node;
    return node;
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    size_t amount_requested = (num_elements * element_size);
    total_memory_requested += amount_requested;

    meta_data * node = malloc(sizeof(meta_data) + amount_requested);
    node->request_size = amount_requested;
    node->filename = filename;
    node->instruction = instruction;
    node->next = head;

    memset(node + sizeof(meta_data), 0, amount_requested)

    head = node;
    return node;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    void * temp = realloc
    return NULL;
}

void mini_free(void *payload) {
    // your code here
}
