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

struct _meta {
    size_t size;
    bool allocated;
    meta * next;
    meta * prev;
};

// Forward declarations of helper functions
size_t _get_greater_power_two(const size_t size);
void _coalesce_left(meta * mta);
void _coalesce_right(meta * mta);
bool _split_set(meta * mta, size_t alloc_size);

// GLOBALS
static meta * HEAD; // Starting point to all the different chunks of allocated memory
static size_t SBRK_GROW = 1;

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

    memset(memory, 0, num * size);
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
    fprintf(stderr, "MALLOC DESIRED SIZE: %zu\n", size);
    meta * itr = HEAD;
    while (itr) {   
        fprintf(stderr, "Looking at %p with space %zu\n", itr, itr->size);
        if (!itr->allocated && (itr->size >= size)) {
            fprintf(stderr, "\tFOUND A SPACE\n");
            // Enough for allocation, check if can split block

            void * return_addr = (void *) itr + sizeof(meta);
            if (_split_set(itr, size)) {
                _coalesce_left(itr);
                return_addr = (void *)(itr->self_tag->next_meta) + sizeof(meta);
            }
            fprintf(stderr, "-------------------------------\n");
            return (void *) itr + sizeof(meta);
        }
        itr = itr->next;
    }
    
    // No existing blocks have enough space, call sbrk

    // Current strategy, avoid calling sbrk every time, allocate the nearest power of 2 for size
    size_t alloc_size = (SBRK_GROW > size) ? SBRK_GROW : size;
    SBRK_GROW *= 2;
    size_t alignment_size = ((alloc_size + 2 * (sizeof(meta) + sizeof(tag)) + 15) / 16) * 16; // Data should be aligned

    // Since we are always getting a power of two that is greater than the input size, we wil have an extra block of free memory
    // Allocate enough space for the alloc_size, and two mem_tags
    fprintf(stderr, "\t\t\tMALLOC: Extending by %zu\n", alignment_size);
    void * data_start = sbrk(alignment_size);
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
    new_tag->next_meta = HEAD;

    if (HEAD) {
        HEAD->prev_tag = new_tag;
    }

    HEAD = new_block;
    _split_set(new_block, size);

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

        itr = itr->self_tag->next_meta;
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
    void * return_ptr = NULL;
    if (!size) {
        // Free if size is 0
        free(ptr);
        return_ptr =  NULL;
    } else if (!ptr) {
        // malloc if ptr is NULL
        return_ptr = malloc(size);
    } else {
        // Everything else
        meta * ptr_meta = (meta *)(ptr - sizeof(meta));

        if (ptr_meta->size == size) {
            // Do nothing is size is the same as current size
            return ptr;
        } else if (ptr_meta->size < size) {
            if (_split_set(ptr_meta, size)) {
                _coalesce_left(ptr_meta);
            }

            // Should I actually do something here?
        } else {
            // ptr-size will always be > size here
            return_ptr = malloc(size);
            if (return_ptr) {
                memcpy(return_ptr, ptr, ptr_meta->size);
                free(ptr);
            }
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
    if (mta != HEAD && !mta->prev_tag) {
        // coalesce with the data behind

        // Get the meta tag of previous block of memory
        fprintf(stderr, "_coalesce_left: [1]\n");
        meta * prev_meta = mta->prev_tag->self_meta;

        // Set the new end tag of the coalesced block
        fprintf(stderr, "_coalesce_left: [2]\n");
        prev_meta->self_tag = mta->self_tag;

        fprintf(stderr, "_coalesce_left: [3]\n");
        // Set the new space available of the coalesced block
        prev_meta->size += (sizeof(tag) + sizeof(meta) + mta->size);
        fprintf(stderr, "_coalesce_left: [SUCCESS]\n");
    }
}

void _coalesce_right(meta * mta) {
    if (mta->self_tag->next_meta && !mta->self_tag->next_meta->allocated) {
        // coalsce with the data ahead

        // Get the meta tag of the next block of memory
        meta * next_meta = mta->self_tag->next_meta;
        
        // Set the new end tag of the coalesced block
        mta->self_tag = next_meta->self_tag;

        // Set the new space available of the coalesced block
        mta->size += (sizeof(tag) + sizeof(meta) + next_meta->size);
    }
}


// Splits the block pointed to at meta into two differenct blocks and allocates mta
bool _split_set(meta * mta, size_t alloc_size) { 
    fprintf(stderr, "Original Size: %zu\n", mta->size);
    if (mta->size >= alloc_size + sizeof(meta) + sizeof(tag)) {
        tag * alloc_tag = mta->self_tag;
        meta * alloc_meta = (meta *) ((void *)alloc_tag - alloc_size - sizeof(meta));

        meta * unalloc_meta = mta;
        unalloc_meta->size = unalloc_meta->size - (sizeof(tag) + sizeof(meta) + alloc_size);
        tag * unalloc_tag = (tag *)(unalloc_meta + sizeof(meta) + unalloc_meta->size);
        // Set alloc data
        alloc_tag->self_meta = alloc_meta;
        alloc_meta->size = alloc_size;
        alloc_meta->allocated = true;
        alloc_meta->self_tag = alloc_tag;
        alloc_meta->prev_tag = unalloc_tag;
        // Set unalloc data
        unalloc_tag->self_meta = unalloc_meta;
        unalloc_tag->next_meta = alloc_meta;

        unalloc_meta->allocated = false;
        unalloc_meta->self_tag = unalloc_tag;

        fprintf(stderr, "unalloc size: %zu, alloc size: %zu\n", unalloc_meta->size, alloc_meta->size);
        fprintf(stderr, "_split_set succeeded!\n");
        return true;
    }

    mta->allocated = true;
    fprintf(stderr, "_split_set did nothing!\n");
    return false;
}
