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
    meta * next_free;
    meta * prev_free;
};

typedef struct _tag {
    meta * self_meta;
} tag;



// Forward declarations of helper functions
void _print_match(meta * mta);
void _coalesce(meta * mta);
void _coalesce_left(meta * mta);
void _coalesce_right(meta * mta);
bool _split_set(meta * mta, size_t alloc_size, bool new_block);

// GLOBALS
static meta * FREE; // List of all free chunks
static size_t SBRK_GROW = 1;
static size_t MALLOC_CAP = 131072;
static void * DATA_START;
static void * DATA_END;

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
    meta * itr = FREE;
    while (itr) {
        if (!itr->allocated && (itr->size >= size)) {         // Enough for allocation, check if can split block
            void * return_addr = (void *) itr + sizeof(meta);
            _split_set(itr, size, false);
            return return_addr;
        }
        itr = itr->next_free;
    }
    
    // No existing blocks have enough space, call sbrk
    // TODO: Come up with a better SBRK method
    while (SBRK_GROW <= size + sizeof(meta) + sizeof(tag)) {
        SBRK_GROW *= 2;
    }
    size_t alloc_size = size + sizeof(meta) + sizeof(tag);
    SBRK_GROW = (SBRK_GROW > MALLOC_CAP) ? MALLOC_CAP: SBRK_GROW;
    if (!DATA_START) {
        DATA_START = sbrk(0);
    }
    void * block_start = sbrk(alloc_size);
    if (block_start == (void *) -1) {
        // sbrk failed which, in turn, means our malloc failed, return NULL.
        return NULL;
    }
    DATA_END = sbrk(0);

    meta * new_block = block_start;
    new_block->size = alloc_size - sizeof(meta) - sizeof(tag);
    new_block->allocated = true;
    new_block->prev_free = NULL;
    new_block->next_free = NULL;
    tag * new_tag = (void *) new_block + sizeof(meta) + new_block->size;
    new_tag->self_meta = new_block;

    void * return_addr = (void *) new_block + sizeof(meta);
    _split_set(new_block, size, true);

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
    if (FREE) {
        mta->next_free = FREE;
        FREE->prev_free = mta;
    }
    FREE = mta;
    _coalesce(mta);
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
        // Have enough space in current area, do nothing
        return ptr;
    }

    return_ptr = malloc(size);
    if (return_ptr) {
        memcpy(return_ptr, ptr, ptr_meta->size);
        free(ptr);
    }
    return return_ptr;
}


void _coalesce(meta * mta) {
    if (!mta->allocated) {
        // left
        if ((void *)mta > DATA_START) {
            _coalesce_left(mta);
        }

        // right
        void * mta_end_attr = (void *)mta + sizeof(meta) + mta->size + sizeof(tag);
        if (mta_end_attr < DATA_END) {
            _coalesce_right(mta);
        }
    }
}

void _coalesce_left(meta * mta) {
    // Assumes left neighbor is within data bounds
    tag * prev_tag = (tag *)((void *)mta - sizeof(tag));
    meta * prev_meta = prev_tag->self_meta;
    if (!prev_meta->allocated) {
        // Redirect pointers to mta
        if (mta->prev_free) {
            mta->prev_free->next_free = mta->next_free;
        }
        if (mta->next_free) {
            mta->next_free->prev_free = mta->prev_free;
        }
        if (FREE == mta) {
            FREE = mta->next_free;
        }

        // Join mta with prev_meta
        prev_meta->size = prev_meta->size + sizeof(tag) + sizeof(meta) + mta->size;
        tag * mta_tag = (tag *)((void *)mta + sizeof(meta) + mta->size);
        mta_tag->self_meta = prev_meta;
        mta = prev_meta;
    }
}

void _coalesce_right(meta * mta) {
    meta * next_meta = (meta *)((void *)mta + sizeof(meta) + mta->size + sizeof(tag));
    if (!next_meta->allocated) {
        // Redirect pointers to next_meta
        if (next_meta->prev_free) {
            next_meta->prev_free->next_free = next_meta->next_free;
        }
        if (next_meta->next_free) {
            next_meta->next_free->prev_free = next_meta->prev_free;
        }
        if (FREE == next_meta) {
            FREE = next_meta->next_free;
        }

        // Join mta with next_meta
        mta->size = mta->size + sizeof(tag) + sizeof(meta) + next_meta->size;
        tag * next_tag = (tag *)((void *)next_meta + sizeof(meta) + next_meta->size);
        next_tag->self_meta = mta;
    }
}


// Splits the block pointed to at meta into two differenct blocks and allocates mta
bool _split_set(meta * mta, size_t alloc_size, bool new_block) { 
    if (mta->size > alloc_size + sizeof(meta) + sizeof(tag)) {
        meta * unalloc_meta = (meta *) ((void *)mta + sizeof(meta) + alloc_size + sizeof(tag));
        unalloc_meta->size = mta->size - alloc_size - sizeof(meta) - sizeof(tag);
        unalloc_meta->allocated = false;
        if (!new_block) {
            unalloc_meta->prev_free = mta->prev_free;
            if (unalloc_meta->prev_free) {
                unalloc_meta->prev_free->next_free = unalloc_meta; // Redirect list neighbor pointers
            }
            unalloc_meta->next_free = mta->next_free;
            if (unalloc_meta->next_free) {
                unalloc_meta->next_free->prev_free = unalloc_meta; // Redirect list neighbor pointers
            }
            if (FREE == mta) {
                FREE = unalloc_meta;
            }
        } else {
            if (FREE) {
                unalloc_meta->next_free = FREE;
                FREE->prev_free = unalloc_meta;
            }
            FREE = unalloc_meta;
        }   

        tag * unalloc_tag = (tag *)((void *)unalloc_meta + sizeof(meta) + unalloc_meta->size);
        unalloc_tag->self_meta = unalloc_meta;
        mta->size = alloc_size;
        mta->allocated = true;
        mta->prev_free = NULL;
        mta->next_free = NULL;
        tag * mta_tag = (tag *)((void *)mta + sizeof(meta) + mta->size);
        mta_tag->self_meta = mta;
        _coalesce(unalloc_meta);
        return true;
    }

    mta->allocated = true;
    mta->prev_free = NULL;
    mta->next_free = NULL;
    return false;
}

void _print_match(meta * mta) {
    // bool printed = false;
    // if (mta->prev) {
    //     fprintf(stderr, "prev->next match: %d || ", mta->prev->next == mta);
    //     printed = true;
    // }

    // if (mta->next) {
    //     fprintf(stderr, "next->prev match: %d ||", mta->next->prev == mta);
    //     printed = true;
    // }

    // if (printed) {
    //     fprintf(stderr, "\n");
    // }
}
