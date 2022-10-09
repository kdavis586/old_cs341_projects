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
    bool in_use;
};

typedef struct _tag {
    size_t size;
} tag;


// Forward declarations of helper functions
meta * _coalesce(meta * mta);
meta * _coalesce_left(meta * mta);
meta * _coalesce_right(meta * mta);
void _split_block(meta * mta, size_t new_mta_size);
void _link_frees(meta * left, meta * right);
meta ** _get_prev(meta * mta);
meta ** _get_next(meta * mta);
void _cycle_check(meta * check);

// GLOBALS
static meta * FREE = NULL; // List of all free chunks
//static size_t SBRK_GROW = 1;
//static size_t MALLOC_CAP = 131072;
static void * DATA_START;
static void * DATA_END;
static size_t MIN_BLOCK_SIZE = sizeof(meta) + 2 * sizeof(meta **) + sizeof(tag);

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
    // Make sure minimum size can store 2 meta ptrs
    if (size < 2 * sizeof(meta *)) {
        size = 2 * sizeof(meta *);
    }

    // See if we have space in the currently allocated memory via best fit
    meta * itr = FREE;
    meta * best = NULL;
    while (itr) {    
        if (itr->size >= size && (!best || itr->size < best->size)) {         // Enough for allocation, check if can split block
            best = itr;
        }
        // meta * temp = itr;
        itr = *_get_next(itr);
        // _coalesce(temp);
    }
    if (best) {
        _split_block(best, size);
        return (void *) best + sizeof(meta);
    }

    // No existing blocks have enough space, call sbrk
    size_t alloc_size = size + sizeof(meta) + sizeof(tag);
    // BUG: Do I want this?
    // if (alloc_size < MALLOC_CAP) {
    //     alloc_size = (alloc_size * 10 > MALLOC_CAP) ? MALLOC_CAP : alloc_size * 1024;
    // }
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
    new_block->in_use = false;
    new_block->size = alloc_size - sizeof(meta) - sizeof(tag);
    *_get_prev(new_block) = NULL;
    *_get_next(new_block) = FREE;
    if (FREE) {
        *_get_prev(FREE) = new_block;
    }
    FREE = new_block;

    tag * new_tag = (void *) new_block + sizeof(meta) + new_block->size;
    new_tag->size = new_block->size;

    _split_block((meta *)new_block, size);
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
    //static int count = 0;
    if (!ptr) return; // Do nothing on NULL pointer.
    meta * mta = ptr - sizeof(meta);

    // // BUG: If present 8 passes, 11 fails and overall slower test times, if not present 8 fails, 11 passes
    // if ((void*)mta + sizeof(meta) + mta->size + sizeof(tag) == DATA_END && DATA_END - DATA_START > 1073741824 / 4) {
    //     // Freeing at data end, reduce heap size
    //     int reduce = (int)(sizeof(meta) + mta->size + sizeof(tag));
    //     sbrk(-reduce);
    //     DATA_END = sbrk(0);
    //     return;
    // }

    mta->in_use = false;
    *_get_prev(mta) = NULL;
    *_get_next(mta) = FREE;

    if (FREE) {
        *_get_prev(FREE) = mta;
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
    
    if (size < 2 * sizeof(meta *)) {
        size = 2 * sizeof(meta *);
    }

    if (!ptr) {
        return malloc(size);
    }

    meta * ptr_meta = (meta *)(ptr - sizeof(meta));
    if (ptr_meta->size >= size) {
        // Have enough space in current area, do nothing
        return ptr;
    }
    
    // TODO: memset 0 on adjacent neighbor if free
    void * end_addr = (void *)ptr_meta + sizeof(meta) + ptr_meta->size + sizeof(tag);
    if (end_addr < DATA_END) {
        meta * next_mem = end_addr;
        
        size_t new_join_size = ptr_meta->size + sizeof(tag) + sizeof(meta) + next_mem->size;
        if (!next_mem->in_use && new_join_size >= size) {
            _link_frees(*_get_prev(next_mem), *_get_next(next_mem));

            if (FREE == next_mem) {
                FREE = *_get_next(next_mem);
            }

            *_get_next(next_mem) = NULL;
            *_get_prev(next_mem) = NULL;

            ptr_meta->size = new_join_size;
            tag * next_tag = (void *)next_mem + sizeof(meta) + next_mem->size;
            next_tag->size = ptr_meta->size;

            memset((void *)next_mem - sizeof(tag), 0, sizeof(tag) + sizeof(meta) + next_mem->size);

            return ptr;
        }
    }


    return_ptr = malloc(size);
    if (return_ptr) {
        memcpy(return_ptr, ptr, ptr_meta->size);
        free(ptr);
    }
    return return_ptr;
}

// Splits the block
void _split_block(meta * mta, size_t new_mta_size) {
    if (mta->size > new_mta_size + MIN_BLOCK_SIZE) {
        // extra_block
        meta * extra_block = (void *)mta + sizeof(meta) + new_mta_size + sizeof(tag);
        extra_block->size = mta->size - new_mta_size - sizeof(tag) - sizeof(meta);
        extra_block->in_use = false;
        *_get_prev(extra_block) = NULL;
        *_get_next(extra_block) = NULL;

        // extra_block tag
        tag * extra_tag = (void *)extra_block + sizeof(meta) + extra_block->size;
        extra_tag->size = extra_block->size;

        // mta
        mta->in_use = true;
        mta->size = new_mta_size;

        // mta tag
        tag * mta_tag = (void *)mta + sizeof(meta) + mta->size;
        mta_tag->size = mta->size;

        // Link free list
        if (FREE == mta || !FREE) {
            FREE = extra_block;
        }

        _link_frees(*_get_prev(mta), extra_block);
        _link_frees(extra_block, *_get_next(mta));

        *_get_next(mta) = NULL;
        *_get_prev(mta) = NULL;
        _coalesce(extra_block);
    } else {
        // mta
        mta->in_use = true;
        _link_frees(*_get_prev(mta), *_get_next(mta));

        if (FREE == mta) {
            FREE = *_get_next(mta);
        }
        

        *_get_next(mta) = NULL;
        *_get_prev(mta) = NULL;
    }
}

meta * _coalesce(meta * mta) {
    if (!mta->in_use) {
        // left
        if ((void *)mta > DATA_START) {
            mta = _coalesce_left(mta);
        }

        // right
        void * end_addr = (void *)mta + sizeof(meta) + mta->size + sizeof(tag);
        if (end_addr < DATA_END) {
            mta = _coalesce_right(mta);
        }
    }

    return mta;
}

meta * _coalesce_left(meta * mta) {
    // Assumes left neighbor is within data bounds
    tag * prev_tag = (void *)mta - sizeof(tag);
    meta * prev_block = (void*)prev_tag - prev_tag->size - sizeof(meta);

    if (!prev_block->in_use) {
        // Redirect pointers to mta
        _link_frees(*_get_prev(mta), *_get_next(mta));

        if (FREE == mta) {
            FREE = *_get_next(mta);
        }

        *_get_next(mta) = NULL;
        *_get_prev(mta) = NULL;

        // Join mta with prev_meta
        prev_block->size = prev_block->size + sizeof(tag) + sizeof(meta) + mta->size;
        tag * mta_tag = (tag *)((void *)mta + sizeof(meta) + mta->size);
        
        mta_tag->size = prev_block->size;
        
        

        mta = prev_block;

        if ((void *)mta > DATA_START) {
            mta = _coalesce_left(mta);
        }
    }

    return mta;
}

meta * _coalesce_right(meta * mta) {
    meta * next_block = (void *)mta + sizeof(meta) + mta->size + sizeof(tag);

    if (!next_block->in_use) {
        // Redirect pointers to mta
        _link_frees(*_get_prev(next_block), *_get_next(next_block));

        if (FREE == next_block) {
            FREE = *_get_next(next_block);
        }

        *_get_next(next_block) = NULL;
        *_get_prev(next_block) = NULL;

        // Join mta with prev_meta
        mta->size = mta->size + sizeof(tag) + sizeof(meta) + next_block->size;
        tag * next_tag = (tag *)((void *)next_block + sizeof(meta) + next_block->size);
        
        next_tag->size = mta->size;

        void * end_addr = (void *)mta + sizeof(meta) + mta->size + sizeof(tag);
        if (end_addr < DATA_END) {
            mta = _coalesce_right(mta);
        }
    }

    return mta;
}

void _link_frees(meta * left, meta * right) {
    if (left) {
        *_get_next(left) = right;
    }
    if (right) {
        *_get_prev(right) = left;
    }
}

meta ** _get_prev(meta * mta) {
    return (meta **)((void *)mta + sizeof(meta));
}

meta ** _get_next(meta * mta) {
    return (meta **)((void *)mta + sizeof(meta) + sizeof(meta **));
}

void _cycle_check(meta * check) {
    meta * itr = FREE;
    bool seen_free = false;
    while (itr) {
        if (itr == check) {
            fprintf(stderr, "alloced block in free list\n");
            exit(1);
        }
        if (itr == FREE) {
            if (seen_free) {
                fprintf(stderr, "Cycle!\n");
                exit(1);
            }

            seen_free = true;
        }

        itr = *_get_next(itr);
    }
}

// meta * _realloc_extend(meta * mta, size_t new_size) {
//     if (_get_extend_size(mta) >= new_size) {
//         if (mta > DATA_START) {
//             tag * prev_mem_tag = (void *)mta - sizeof(tag);
//             meta * prev_mem = (void *)tag - tag->size - sizeof(meta);

//             if (!prev_mem->in_use) {
//                 // TODO: Some realloc specific joining in here
//                 _link_frees(*_get_prev(prev_mem), *_get_next(prev_mem));

//                 if (FREE == prev_mem) {
//                     FREE = *_get_next(prev_mem);
//                 }

//                 *_get_prev(prev_mem) = NULL;
//                 *_get_next(prev_mem) = NULL;

//                 prev_block->size = prev_block->size + sizeof(tag) + sizeof(meta) + mta->size;
//                 tag * mta_tag = (tag *)((void *)mta + sizeof(meta) + mta->size);
                
//                 mta_tag->size = prev_block->size;


//                 extended = true;
//             }
//         }

//         void * end_addr = (void *)mta + sizeof(meta) + mta->size + sizeof(tag);
//         if (end_addr < DATA_END) {
//             meta * next_mem = end_addr;
//             if (!next_mem->in_use) {
//                 // TODO: Some realloc specific joining in here
//                 extended = true;
//             }
//         }
//     }

//     return mta;
// }

// size_t _get_extend_size(meta * mta) {
//     size_t extend_size = mta->size;

//     if (mta > DATA_START) {
//         tag * prev_mem_tag = (void *)mta - sizeof(tag);
//         meta * prev_mem = (void *)tag - tag->size - sizeof(meta);

//         if (!prev_mem->in_use) {
//             // TODO: Some realloc specific joining in here
//             extend_size += prev_mem->size + sizeof(tag) + sizeof(meta);
//         }
//     }

//     void * end_addr = (void *)mta + sizeof(meta) + mta->size + sizeof(tag);
//     if (end_addr < DATA_END) {
//         meta * next_mem = end_addr;
//         if (!next_mem->in_use) {
//             // TODO: Some realloc specific joining in here
//             extend_size += sizeof(tag) + sizeof(meta) + next_mem->size;
//         }
//     }

//     return extend_size;
// }