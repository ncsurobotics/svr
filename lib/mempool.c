/**
 * \file
 * \brief Memory pool
 */

#include "svr.h"

/**
 * Align all allocations on a byte boundary of this alignment
 */
#define ALLOC_ALIGNMENT 2

static SVR_BlockAllocator* descriptor_allocator = NULL;
static SVR_Arena* SVR_Arena_allocExternal(size_t size);

/**
 * \defgroup MemPool Memory pool
 * \ingroup Util
 * \brief Memory pool for small short lived objects
 * \{
 */

/**
 * \brief Initialize the SVR_MemPool component
 *
 * Initialize the SVR_MemPool component
 */
void SVR_MemPool_init(void) {
    descriptor_allocator = SVR_BlockAlloc_newAllocator(sizeof(SVR_Arena), 4);
}

/**
 * \brief Close the SVR_MemPool component
 *
 * Close the SVR_MemPool component
 */
void SVR_MemPool_close(void) {
    SVR_BlockAlloc_freeAllocator(descriptor_allocator);
}

/**
 * \brief Get a new allocation
 *
 * Return a new allocation of fixed capacity that can be used with
 * SVR_MemPool_write, SVR_MemPool_reserve, and SVR_MemPool_strdup. When the allocation is no
 * longer needed, it should be passed to SVR_MemPool_free
 *
 * \return The new allocation object
 */
SVR_Arena* SVR_Arena_alloc(SVR_BlockAllocator* allocator) {
    SVR_Arena* alloc = SVR_BlockAlloc_alloc(descriptor_allocator);

#ifdef SVR_DUMMY_ALLOC
    alloc->base = NULL;
    alloc->allocator = NULL;
#else
    alloc->base = SVR_BlockAlloc_alloc(allocator);
    alloc->allocator = allocator;
#endif
    alloc->write_index = 0;
    alloc->next = NULL;

    return alloc;
}

static SVR_Arena* SVR_Arena_allocExternal(size_t size) {
    SVR_Arena* alloc = SVR_BlockAlloc_alloc(descriptor_allocator);

    alloc->base = malloc(size);
    alloc->allocator = NULL;
    alloc->write_index = 0;
    alloc->next = NULL;

    return alloc;
}

/**
 * \brief Free an allocation
 *
 * Free an allocation previously returned by SVR_MemPool_alloc
 *
 * \param alloc The allocation to free
 */
void SVR_Arena_free(SVR_Arena* alloc) {
    if(alloc->next) {
        SVR_Arena_free(alloc->next);
    }

    if(alloc->allocator == NULL) {
        free(alloc->base);
    } else {
        SVR_BlockAlloc_free(alloc->allocator, alloc->base);
    }

    SVR_BlockAlloc_free(descriptor_allocator, alloc);
}

/**
 * \brief Write to an allocation
 *
 * Copy data into the allocation and return a pointer to the beginning of the
 * written data. This is similar to a malloc followed by a memcpy.
 *
 * \param alloc The allocation to copy to
 * \param data A pointer to the data to copy
 * \param size The number of bytes to copy from data
 * \return A pointer to the beginning of the copied data
 */
void* SVR_Arena_write(SVR_Arena* alloc, const void* data, size_t size) {
    void* p = SVR_Arena_reserve(alloc, size);
    memcpy(p, data, size);

    return p;
}

/**
 * \brief Copy a string to the allocation
 *
 * Copy a null-terminated string to the allocation and return a pointer to it.
 * This call is equivalent to strdup, but for SVR_MemPool allocations
 *
 * \param alloc The allocation to copy to
 * \param s The null-terminated string to copy
 * \return A pointer to the copy
 */
void* SVR_Arena_strdup(SVR_Arena* alloc, const char* s) {
    return SVR_Arena_write(alloc, s, strlen(s) + 1);
}

/**
 * \brief Prepare a formatted string
 *
 * Writes a formatted string as sprintf, but allocates space dynamically
 *
 * \param alloc SVR_Arena to use for space
 * \param format Format specifier, same as given to printf family
 * \param ... arguments to format string
 * \return Pointer to formatted string
 */
void* SVR_Arena_sprintf(SVR_Arena* alloc, const char* format, ...) {
#ifdef SVR_DUMMY_ALLOC
    void* p = NULL;
    va_list ap;
    int n;

    va_start(ap, format);
    n = vsnprintf(p, 0, format, ap);
    va_end(ap);

    p = SVR_Arena_reserve(alloc, n + 1);

    va_start(ap, format);
    vsnprintf(p, n + 1, format, ap);
    va_end(ap);

    return p;
#else
    size_t space = SVR_BlockAlloc_getBlockSize(alloc->allocator) - alloc->write_index;
    void* p = ((uint8_t*)alloc->base) + alloc->write_index;
    va_list ap;
    int n;

    va_start(ap, format);
    n = vsnprintf(p, space, format, ap);
    va_end(ap);

    /* If the above vsnprintf call did not truncate the output then this call
       will simply claim the space used, otherwise it will make the necessary
       space so we can vsnprintf successfully */
    p = SVR_Arena_reserve(alloc, n + 1);

    /* Output was truncated, try with the new reserved space */
    if(n >= space) {
        va_start(ap, format);
        vsnprintf(p, n + 1, format, ap);
        va_end(ap);
    }

    return p;
#endif
}

/**
 * \brief Reserve space in an allocation
 *
 * Reserve space in the allocation that can be written to by the caller instead
 * of by one of SVR_MemPool_write or SVR_MemPool_strup. This call is therefore
 * analogous to malloc.
 *
 * \param alloc The allocation to reserve space in
 * \param size The number of bytes to reserve
 * \return A pointer to the reserved space
 */
void* SVR_Arena_reserve(SVR_Arena* alloc, size_t size) {
#ifdef SVR_DUMMY_ALLOC
    while(alloc->next) {
        alloc = alloc->next;
    }

    alloc->next = SVR_Arena_allocExternal(size);
    return alloc->next->base;
#else
    SVR_BlockAllocator* allocator = alloc->allocator;
    size_t block_size = SVR_BlockAlloc_getBlockSize(allocator);
    bool found;
    void* p;

    /* Round up to the nearest boundary to keep alignment if not already aligned */
    if(size % ALLOC_ALIGNMENT > 0) {
        size += (ALLOC_ALIGNMENT - (size % ALLOC_ALIGNMENT));
    }

    /* Find a chunk with enough free space or exhaust the list */
    found = false;
    while(alloc->next) {
        if(alloc->write_index + size < block_size) {
            found = true;
            break;
        }

        alloc = alloc->next;
    }

    /* Allocate a new block if necessary */
    if(!found) {
        if(size > block_size) {
            /* Too big for a block, allocate directly */
            alloc->next = SVR_Arena_allocExternal(size);
        } else {
            alloc->next = SVR_Arena_alloc(allocator);
        }

        alloc = alloc->next;
    }

    p = ((uint8_t*)alloc->base) + alloc->write_index;
    alloc->write_index += size;

    return p;
#endif
}

/** \} */
