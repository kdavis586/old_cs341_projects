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
static meta * HEAD;               // Starting point to all the different chunks of allocated memory

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
    return NULL;
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
    // implement malloc!
    
    // Current strategy, avoid calling sbrk every time, allocate the nearest power of 2 for size
    // size_t alloc_size = _get_greater_power_two(size);
    // size_t alignment_size = (alloc_size + 2 * sizeofj(mem_tag) + 15) / 16;
    // // Since we are always getting a power of two that is greater than the input size, we wil have an extra block of free memory
    // // Allocate enough space for the alloc_size, and two mem_tags

    // void * data_start = sbrk(alloc_size + 2 * sizeof(mem_tag));
    // if ((int) data_start == -1) {
    //     // sbrk failed which, in turn, means our malloc failed, return NULL.
    //     return NULL;
    // }

    // mem_tag * alloc_tag = (mem_tag *) data_start;
    // alloc_tag->size = size;
    // alloc_tag->allocated = true;
    // alloc_tag->next = HEAD;
    
    // mem_tag * free_tag = (mem_tag *) (data_start + sizeof(mem_tag) + size);
    // free_tag->size = alloc_size - size - 2 * sizeof(mem_tag);
    // free_tag->allocated = false;
    // free_tag->next = alloc_tag;
    // HEAD = free_tag;

    // return (void *) alloc_tag + sizeof(mem_tag);
    return NULL;
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
    // implement free!
    if (!ptr) return; // Do nothing on NULL pointer.

    meta * to_search = (meta *) (ptr - sizeof(meta));
    meta * itr = HEAD;

    while (itr) {
        if (itr == to_search) {
            // Handle up to double coalesce

            // First things first, make self section available for allocation
            itr->allocated = false;

            //                                   if itr is not the HEAD, then there has to be a previous meta tag...
            if (itr != HEAD && !itr->prev_tag->self_meta->allocated) {
                // coalesce with the data behind

                // Get the meta tag of previous block of memory
                meta * prev_meta = itr->prev_tag->self_meta;

                // Set the new end tag of the coalesced block
                prev_meta->self_tag = itr->self_tag;

                // Set the new space available of the coalesced block
                prev_meta->size += (sizeof(tag) + sizeof(meta) + itr->size);

                // Set itr to the prev_meta, so code below will have proper reference to work with
                itr = prev_meta;
            }

            if (itr->self_tag->next && !itr->self_tag->next->allocated) {
                // coalsce with the data aHEAD

                // Get the meta tag of the next block of memory
                meta * next_meta = itr->self_tag->next;
                
                // Set the new end tag of the coalesced block
                itr->self_tag = next_meta->self_tag;

                // Set the new space available of the coalesced block
                itr->size += (sizeof(tag) + sizeof(meta) + next_meta->size);
            }

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
    // implement realloc!
    return NULL;
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
