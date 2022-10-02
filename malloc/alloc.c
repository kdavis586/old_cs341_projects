/**
 * malloc
 * CS 341 - Fall 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct _meta meta;
typedef struct _tag tag; 

struct _tag {
    meta * self_meta;
    meta * next;
};

struct _meta {
    size_t size;
    bool allocated;
    tag * self_tag;
    tag * prev_tag;
};



// GLOBALS
static meta * HEAD; // Starting point to all the different chunks of allocated memory

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void * memory;

    if (!(memory = malloc(num * size))) {
        return NULL;
    }

    memcpy(memory, 0, num * size);
    return memory;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // See if we have space in the currently allocated memory
    meta * itr = HEAD;
    while (itr) {   
        if (!itr->allocated && (itr->size >= size)) {
            // Enough for allocation, check if can split block

            if (_split_block(itr, size)) {
                // Enough space for data about remaining block, create new link
                // Potentially coalesce new split with neighbor is free
                _coalesce_right(split_meta);
            } else {
                itr->allocated = true;
            }

            return (void *) itr + sizeof(meta);
        }
        itr = itr->self_tag->next;
    }

    // No existing blocks have enough space, call sbrk

    // Current strategy, avoid calling sbrk every time, allocate the nearest power of 2 for size
    size_t alloc_size = _get_greater_power_two(size);
    size_t alignment_size = (alloc_size + 2 * sizeof(mem_tag) + 15) / 16; // Data should be aligned

    // Since we are always getting a power of two that is greater than the input size, we wil have an extra block of free memory
    // Allocate enough space for the alloc_size, and two mem_tags

    void * data_start = sbrk(alignment_size + (sizeof(meta) + sizeof(tag));
    if ((int) data_start == -1) {
        // sbrk failed which, in turn, means our malloc failed, return NULL.
        return NULL;
    }

    meta * new_block = data_start;
    tag * new_tag = data_start + sizeof(meta) + alignment_size;

    new_block->size = alignment_size;
    new_block->allocated = true;
    new_block->self_tag = new_tag;
    
    new_tag->self_meta = new_block;
    new_tag->next = HEAD;

    HEAD = new_block;

    if (_split_block(new_block, size) {
        _coalesce_right(new_block->self_tag->next);
    }

    return (void *) new_block + sizeof(meta);
}


/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    if (!ptr) return; // Do nothing on NULL pointer.

    meta * to_search = (meta *) (ptr - sizeof(meta));
    meta * itr = HEAD;

    while (itr) {
        if (itr == to_search) {
            // Handle up to double coalesce

            // First things first, make self section available for allocation
            itr->allocated = false;

            // Coalesce if needed
            _coalesce_left(itr);
            _coalesce_right(itr);

            // No need to continue the while loop
            break;
        }

        itr = itr->self_tag->next;
    }
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    void * return_ptr;

    if (!size) {
        free(ptr);
        return_ptr =  NULL;
    } else if (!ptr) {
        return_ptr = malloc(size);
    } else if (ptr->size == size) {
        return ptr;
    } else if (ptr->size < size) {
        if (_split_block(ptr, size)) {
            _coalesce_right(ptr->self_tag->next);
        }

        // Should I actually do something here?
    } else {
        // ptr-size will always be > size here
        return_ptr = malloc(size);
        if (return_ptr) {
            meta * mta = ptr - sizeof(meta);
            memcpy(return_ptr, ptr, mta->size);
            free(ptr);
        }
    }

    return return_ptr;
}


/**
* Gets the power of two that is stricly greater thant the input size
*
* @param size
*   The size that we want to get a greater power of two of.
*
* @return
*   A size_t reprenting the power of 2 that is explicitly greater than the input size.
*/
size_t _get_greater_power_two(const size_t size) {
    size_t power = 1;

    while (power <= size) {
        power *= 2;
    }

    return power;
}

void _coalesce_left(meta * mta) {
    if (mta != HEAD && !mta->prev_tag->self_meta->allocated) {
        // coalesce with the data behind

        // Get the meta tag of previous block of memory
        meta * prev_meta = mta->prev_tag->self_meta;

        // Set the new end tag of the coalesced block
        prev_meta->self_tag = mta->self_tag;

        // Set the new space available of the coalesced block
        prev_meta->size += (sizeof(tag) + sizeof(meta) + mta->size);
    }
}

void _coalesce_right(meta * mta) {
    if (mta->self_tag->next && !mta->self_tag->next->allocated) {
        // coalsce with the data ahead

        // Get the meta tag of the next block of memory
        meta * next_meta = mta->self_tag->next;
        
        // Set the new end tag of the coalesced block
        mta->self_tag = next_meta->self_tag;

        // Set the new space available of the coalesced block
        mta->size += (sizeof(tag) + sizeof(meta) + next_meta->size);
    }
}


// Splits the block pointed to at meta into two differenct blocks and allocates mta
bool _split_block(meta * mta, size_t alloc_size) {
    if (mta->size - size >= sizeof(meta) + sizeof(tag)) {
        tag * alloc_tag = (void *) mta + sizeof(meta) + alloc_size;
        meta * split_meta = (void *) alloc_tag + sizeof(tag);

        // Assign split block meta and tag
        split_meta->size = mta->size - alloc_size - sizeof(tag) - sizeof(meta);
        split_meta->allocated = false;
        split_meta->self_tag = mta->self_tag;
        split_meta->self_tag->self_meta = split_meta;
        split_meta->prev_tag = alloc_tag;
        
        // Assign new mta values
        mta->size = alloc_size;
        mta->allocated = true;
        mta->self_tag = alloc_tag;
        mta->self_tag->self_meta = mta;
        mta->self_tag->next = split_meta;

        return true;
    }

    return false;
}
