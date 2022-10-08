/**
 * malloc
 * CS 341 - Fall 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

typedef struct _meta meta;
typedef struct _free_meta free_meta; 

struct _meta {
    size_t size;
    char allocated;
};

struct _free_meta {
    size_t size;
    bool allocated;
    free_meta * prev_free;
    free_meta * next_free;
};

typedef struct _tag {
    meta * self_meta;
} tag;


// Forward declarations of helper functions
void _print_free_meta(free_meta * free_mta);
void _coalesce(free_meta * free_mta);
void _coalesce_left(free_meta * free_mta);
void _coalesce_right(free_meta * free_mta);
bool _split_set(meta * mta, size_t alloc_size, bool new_block);
void _link_frees(free_meta * left, free_meta * right);

// GLOBALS
static free_meta * FREE; // List of all free chunks
//static size_t SBRK_GROW = 1;
//static size_t MALLOC_CAP = 131072;
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

// XXX: Test 5 cycles on free list search

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
    // See if we have space in the currently allocated memory (uses best fit)
    free_meta * itr = FREE;
    free_meta * best = NULL;
    while (itr) {
        if (!itr->allocated && (itr->size >= size) && (!best || itr->size < best->size)) {         // Enough for allocation, check if can split block
            best = itr;
        }
        itr = itr->next_free;
    }
    if (best) {
        void * return_addr = (void *) best + sizeof(meta);
        _split_set((meta *)best, size, false);

        return return_addr;
    }

    // No existing blocks have enough space, call sbrk
    size_t data_size = (size > 2 * sizeof(meta *)) ? size : 2 * sizeof(meta *);
    size_t alloc_size = data_size + sizeof(meta) + sizeof(tag);
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
    new_block->size = data_size;

    tag * new_tag = (void *) new_block + sizeof(meta) + new_block->size;
    new_tag->self_meta = new_block;

    void * return_addr = (void *) new_block + sizeof(meta);
    _split_set(new_block, data_size, true);
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
    free_meta * free_mta = (free_meta *)(ptr - sizeof(meta));
    // if ((void*)free_mta + sizeof(meta) + free_mta->size + sizeof(tag) == DATA_END && free_mta->size > 1024) {
    //     // Freeing at data end, reduce heap size
    //     int reduce = -(sizeof(meta) + mta->size + sizeof(tag));
    //     sbrk(reduce);
    //     DATA_END = sbrk(0);
    //     return;
    // }
    free_mta->allocated = false;
    free_mta->prev_free = NULL;
    free_mta->next_free = NULL;

    if (FREE) {
        free_mta->next_free = FREE;
        FREE->prev_free = free_mta;
    }
    FREE = free_mta;
    
    _coalesce(free_mta);
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


void _coalesce(free_meta * free_mta) {
    if (!free_mta->allocated) {
        // left
        if ((void *)free_mta > DATA_START) {
            _coalesce_left(free_mta);
        }

        // right
        void * mta_end_attr = (void *)free_mta + sizeof(meta) + free_mta->size + sizeof(tag);
        if (mta_end_attr < DATA_END) {
            _coalesce_right(free_mta);
        }
    }
}

void _coalesce_left(free_meta * free_mta) {
    // Assumes left neighbor is within data bounds
    tag * prev_tag = (tag *)((void *)free_mta - sizeof(tag));
    free_meta * prev_cast = (free_meta *)(prev_tag->self_meta);

    if (!prev_cast->allocated) {
        // Redirect pointers to mta
        _link_frees(free_mta->prev_free, free_mta->next_free);
        if (FREE == free_mta) {
            assert(free_mta->prev_free == NULL);
            FREE = free_mta->next_free;

            if (FREE) {
                FREE->prev_free = NULL;
            }
        }
        free_mta->next_free = NULL;
        free_mta->prev_free = NULL;

        // Join mta with prev_meta
        prev_cast->size = prev_cast->size + sizeof(tag) + sizeof(meta) + free_mta->size;
        tag * mta_tag = (tag *)((void *)free_mta + sizeof(meta) + free_mta->size);
        mta_tag->self_meta = (meta *)prev_cast;
        free_mta = prev_cast;
    }
}

void _coalesce_right(free_meta * free_mta) {
    free_meta * next_cast = (free_meta *)((void *)free_mta + sizeof(meta) + free_mta->size + sizeof(tag));

    if (!next_cast->allocated) {
        // Redirect pointers to mta
        _link_frees(next_cast->prev_free, next_cast->next_free);
        if (FREE == next_cast) {
            assert(next_cast->prev_free == NULL);
            FREE = next_cast->next_free;

            if (FREE) {
                FREE->prev_free = NULL;
            }
        }
        next_cast->next_free = NULL;
        next_cast->prev_free = NULL;

        // Join mta with next_meta
        free_mta->size = free_mta->size + sizeof(tag) + sizeof(meta) + next_cast->size;

        tag * next_tag = (tag *)((void *)next_cast + sizeof(meta) + next_cast->size);
        next_tag->self_meta = (meta *)free_mta;
    }
}


// Splits the block pointed to at meta into two differenct blocks and allocates mta
bool _split_set(meta * mta, size_t alloc_size, bool new_block) {
    free_meta * mta_cast = (free_meta *)mta;
    if (mta->size > alloc_size + sizeof(meta) + sizeof(tag)) {
        // Only allocating part of block, init new free block and link free neighbors
        free_meta * unalloc_meta = (free_meta *)((void *)mta + sizeof(meta) + alloc_size + sizeof(tag));
        unalloc_meta->size = mta->size - alloc_size - sizeof(meta) - sizeof(tag);
        unalloc_meta->allocated = false;
        unalloc_meta->prev_free = NULL;
        unalloc_meta->next_free = NULL;

        tag * unalloc_tag = (tag *)((void *)unalloc_meta + sizeof(meta) + unalloc_meta->size);
        unalloc_tag->self_meta = (meta *)unalloc_meta;

        if (!new_block) {
            // Free neighbors might exist, link them
            if (FREE == mta_cast) {
                assert(mta_cast->prev_free == NULL);
                FREE = unalloc_meta;
                assert(FREE->prev_free == NULL);
            }
            _link_frees(mta_cast->prev_free, FREE);
            _link_frees(FREE, mta_cast->next_free);
        } else {
            // Brand new block, no free neighbors
            if (FREE) {
                unalloc_meta->next_free = FREE;
                if (FREE) {
                    FREE->prev_free = unalloc_meta;
                }
            }
            assert(unalloc_meta->prev_free == NULL);
            FREE = unalloc_meta;
        }

        _coalesce(unalloc_meta);
    } else if (!new_block) {
        // Allocating whole block
        if (FREE == mta_cast) {
            FREE = mta_cast->next_free;
            if (FREE) {
                FREE->prev_free = NULL;
            }
        }
        _link_frees(mta_cast->prev_free, mta_cast->next_free);
    }

    // Set mta to allocated
    mta_cast->allocated = true;
    mta_cast->prev_free = NULL;
    mta_cast->next_free = NULL;

    tag * mta_tag = (tag *)((void *)mta + sizeof(meta) + mta_cast->size);
    mta_tag->self_meta = mta;

    return false;
}

void _print_free_meta(free_meta * free_mta) {
    fprintf(stderr, "meta info\n- addr: %p\n- size: %zu\n- allocated:%d\n- prev_free: %p\n- next_free: %p\n", \
    free_mta, free_mta->size, free_mta->allocated, free_mta->prev_free, free_mta->next_free);
}

void _link_frees(free_meta * left, free_meta * right) {
    if (left) {
        left->next_free = right;
    }
    if (right) {
        right->prev_free = left;
    }
}
