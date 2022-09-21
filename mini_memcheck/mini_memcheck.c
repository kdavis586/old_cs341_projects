/**
 * mini_memcheck
 * CS 341 - Fall 2022
 */
#include "mini_memcheck.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

meta_data * head = NULL;
size_t total_memory_requested = 0;
size_t total_memory_freed = 0;
size_t invalid_addresses = 0;

bool _check_valid_address(const void *payload);

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

    return ((void *)node + sizeof(meta_data));
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

    memset((void *)node + sizeof(meta_data), 0, amount_requested);

    head = node;
    return ((void *)node + sizeof(meta_data));
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    if (!_check_valid_address(payload)) {
        invalid_addresses++;
        return NULL;
    } else if (!payload) {
        return mini_malloc(request_size, filename, instruction);
    } else if (!request_size) {
        mini_free(payload);
        return NULL;
    } 

    meta_data * mta_data = payload - sizeof(meta_data);
    total_memory_requested -= mta_data->request_size;
    meta_data * temp = (meta_data *) realloc(mta_data, sizeof(meta_data) + request_size);
    if (!temp) {
        // realloc failed, mta_data->request_size didnt get changed
        total_memory_requested += mta_data->request_size;
        return NULL;
    }

    total_memory_requested += request_size;

    if (mta_data != temp) {
        // Memory was moved, restore the link
        meta_data * start = head;
        while (start) {
            if (start->next == mta_data) {
                start->next = temp;
                temp->next = mta_data->next;
                break;
            }

            start = start->next;
        }

        // Do not need anything here because payload will always be found, guarenteed by _check_valid_address
    }
    
    mta_data = temp;
    temp = NULL;
    mta_data->request_size = request_size;
    mta_data->filename = filename;
    mta_data->instruction = instruction;
    // no need to edit this nodes place in the linked list

    return (void *)mta_data + sizeof(meta_data);
}

void mini_free(void *payload) {
    if (_check_valid_address(payload)) {
        if (payload) {
            meta_data * mta_data = (meta_data *)(payload - sizeof(meta_data));
            total_memory_freed += (mta_data->request_size);

            // unlink payload before freeing
            meta_data * true_head = head;
            meta_data * prev = NULL;
            meta_data * to_free = NULL;
            while (head) {
                if (head == mta_data) {
                    if (prev) {
                        prev->next = head->next;
                        to_free = head;
                        head = true_head;
                    } else {
                        // found match at the start of the LL, edit head differently
                        to_free = head;
                        head = head->next;
                    }
                    break;
                }
                prev = head;
                head = head->next;
            } // Will always be unlinked because _check_valid_address was successful
            free(to_free);
            to_free = NULL;
        }
    } else {
        invalid_addresses++;
    }
}

// Checks to see if the input payload is a valid address to free in the linked list
bool _check_valid_address(const void *payload) {
    if (!payload) {
        // NULL is always a valid address to realloc or free
        return true;
    }

    meta_data * start = head;
    meta_data * mta_data = (meta_data * )(payload - sizeof(meta_data));
    while (start) {
        if (start == mta_data) {
            return true;
        }

        start = start->next;
    }

    return false;
}
