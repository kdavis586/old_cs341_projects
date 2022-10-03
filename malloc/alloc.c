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
void _coalesce_left(meta * mta);
void _coalesce_right(meta * mta);
bool _split_set(meta * mta, size_t alloc_size);

// GLOBALS
static meta * HEAD; // Starting point to all the different chunks of allocated memory
static size_t SBRK_GROW = 1;
static size_t MALLOC_CAP = 131072;

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
    meta * itr = HEAD;
    while (itr) {
        if (!itr->allocated && (itr->size >= size)) {
            // Enough for allocation, check if can split block
            void * return_addr = (void *) itr + sizeof(meta);
            if (_split_set(itr, size)) {
                _coalesce_left(itr);
                return_addr = (void *)(itr->next) + sizeof(meta);
            }
            return return_addr;
        }

        itr = itr->next;
    }
    
    // No existing blocks have enough space, call sbrk
    while (SBRK_GROW < size + sizeof(meta)) {
        SBRK_GROW *= 2;
    }
    size_t alloc_size = SBRK_GROW;
    SBRK_GROW = (SBRK_GROW > MALLOC_CAP) ? MALLOC_CAP: SBRK_GROW * 2;
    
    void * data_start = sbrk(alloc_size);

    if (data_start == (void *) -1) {
        // sbrk failed which, in turn, means our malloc failed, return NULL.
        return NULL;
    }

    meta * new_block = data_start;

    new_block->size = alloc_size - sizeof(meta);
    new_block->allocated = true;
    new_block->next = HEAD;

    if (HEAD) {
        HEAD->prev = new_block;
    }

    HEAD = new_block;

    void * return_addr = (void *) new_block + sizeof(meta);
    if (_split_set(new_block, size)) {
        return_addr = (void *) (new_block->next) + sizeof(meta);
    }

    return return_addr;
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
    meta * mta = ptr - sizeof(meta);
    mta->allocated = false;
    // Coalesce if needed
    _coalesce_left(mta);
    _coalesce_right(mta);
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
        free(ptr);
        return NULL;
    } 
    
    if (!ptr) {
        return malloc(size);
    }

    meta * ptr_meta = (meta *)(ptr - sizeof(meta));
    if (ptr_meta->size >= size) {
        // Do nothing is size is the same as current size
        return ptr;
    }

    return_ptr = malloc(size);
    if (return_ptr) {
        meta * return_meta = return_ptr - sizeof(meta);
        memcpy(return_ptr, ptr, ptr_meta->size);
        free(ptr);
    }
    return return_ptr;
}


void _coalesce_left(meta * mta) {
    if (mta->prev && !mta->prev->allocated) {
        _coalesce_right(mta->prev);
        mta = mta->prev;
    }
}

void _coalesce_right(meta * mta) {
    if (mta->next && !mta->next->allocated) {
        mta->size += mta->next->size;
        mta->next = mta->next->next;
    }
}


// Splits the block pointed to at meta into two differenct blocks and allocates mta
bool _split_set(meta * mta, size_t alloc_size) { 
    if (mta->size > alloc_size + sizeof(meta)) {
        // Set alloc data
        meta * alloc_meta = (void *)mta + (mta->size - alloc_size);
        alloc_meta->size = alloc_size;
        alloc_meta->allocated = true;
        alloc_meta->next = mta->next;
        alloc_meta->prev = mta;

        // Set unalloc data
        meta * unalloc_meta = mta;
        unalloc_meta->size = unalloc_meta->size - alloc_size - sizeof(meta);
        unalloc_meta->allocated = false;
        unalloc_meta->next = alloc_meta;

        return true;
    }

    mta->allocated = true;
    return false;
}
