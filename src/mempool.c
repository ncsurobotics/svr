
#include "svr.h"

/**
 * Align all allocations on a byte boundary of this alignment
 */
#define ALLOC_ALIGNMENT 2

static BlockAllocator* descriptor_allocator = NULL;

static SVR_Arena* SVR_Arena_allocExternal(size_t size);

/**
 * \brief Initialize the SVR_MemPool component
 *
 * Initialize the SVR_MemPool component
 */
void SVR_MemPool_init(void) {
    descriptor_allocator = BlockAlloc_newAllocator(sizeof(SVR_Arena));
}

/**
 * \brief Close the SVR_MemPool component
 *
 * Close the SVR_MemPool component
 */
void SVR_MemPool_close(void) {
    BlockAlloc_freeAllocator(descriptor_allocator);
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
SVR_Arena* SVR_Arena_alloc(BlockAllocator* allocator) {
    SVR_Arena* alloc = BlockAlloc_alloc(descriptor_allocator);

    alloc->base = BlockAlloc_alloc(allocator);
    alloc->allocator = allocator;
    alloc->write_index = 0;
    alloc->next = NULL;

    return alloc;
}

static SVR_Arena* SVR_Arena_allocExternal(size_t size) {
    SVR_Arena* alloc = BlockAlloc_alloc(descriptor_allocator);

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
    if (alloc->next) {
        SVR_Arena_free(alloc->next);
    }

    if (alloc->allocator == NULL) {
        free(alloc->base);
    } else {
        BlockAlloc_free(alloc->allocator, alloc->base);
    }

    BlockAlloc_free(descriptor_allocator, alloc);
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
    void* p;

    /* Round up to the nearest boundary to keep alignment if not already aligned */
    if (size % ALLOC_ALIGNMENT > 0) {
        size += (ALLOC_ALIGNMENT - (size % ALLOC_ALIGNMENT));
    }

    /* Find a chunk with enough free space or exhaust the list */
    while(alloc->next) {
        if (alloc->write_index + size < BlockAlloc_getBlockSize(alloc->allocator)) {
            break;
        }

        alloc = alloc->next;
    }

    /* Allocate a new block if necessary */
    if (alloc->write_index + size >= BlockAlloc_getBlockSize(alloc->allocator)) {
        if(size > BlockAlloc_getBlockSize(alloc->allocator)) {
            /* Too big for a block, allocate directly */
            alloc->next = SVR_Arena_allocExternal(size);
        } else {
            alloc->next = SVR_Arena_alloc(alloc->allocator);
        }

        alloc = alloc->next;
    }

    p = ((uint8_t*)alloc->base) + alloc->write_index;
    alloc->write_index += size;

    return p;
}

/** \} */
