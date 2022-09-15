/**
 * mini_memcheck
 * CS 341 - Fall 2022
 */
#include "mini_memcheck.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    meta_data * node = malloc(sizeof(meta_data) + request_size);
    if (!node) {
        return NULL;
    }

    total_memory_requested += request_size;
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
    meta_data * node = malloc(sizeof(meta_data) + amount_requested);
    if (!node) {
        return NULL;
    }

    total_memory_requested += amount_requested;
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
    if (!_check_valid_address(payload)) {
        invalid_addresses++;
        return NULL;
    }

    
    void * temp = realloc(payload, sizeof(meta_data) + request_size);
    if (!temp) {
        return NULL;
    }

    total_memory_requested -= payload->request_size;
    total_memory_requested += request_size

    if (payload != temp) {
        // Memory was moved, restore the link
        meta_data * start = head;
        while (start) {
            if (start->next == payload) {
                start->next = temp;
                break;
            }

            start = start->next;
        }

        // Do not need anything here because payload will always be found, guarenteed by _check_valid_address
    }
    
    payload = temp;
    (meta_data *) payload->request_size = request_size;
    (meta_data *) payload->filename = filename;
    (meta_data *) payload->instruction = instruction;
    // no need to edit this nodes place in the linked list

    return payload;
}

void mini_free(void *payload) {
    if (_check_valid_address(payload)) {
        total_memory_freed += (sizeof(meta_data) + (meta_data *) payload->request_size);
        free(payload);
    } else {
        invalid_addresses++;
    }
}

// Checks to see if the input payload is a valid address to free in the linked list
bool _check_valid_address(void *payload) {
    if (!payload) {
        // NULL is always a valid address to realloc or free
        return true;
    }

    memset * start = head;
    while (start) {
        if (start == payload) {
            return true;
        }

        start = start->next;
    }

    return false;
}
