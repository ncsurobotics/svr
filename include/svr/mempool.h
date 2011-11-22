/**
 * \file
 */

#ifndef __SVR_ARENA_H
#define __SVR_ARENA_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#include <seawolf/list.h>

#include <svr/forward.h>

/**
 * \addtogroup MemPool
 * \{
 */

/**
 * \private
 * \brief A memory pool arena which allocations can be made from
 *
 * A MemPool arena is a linked list of SVR_Arenas, each acting as
 * a chunk in the total arena. The size of each chunk is determined
 * by the parameters passed to SVR_MemPool_new. If a resevation is
 * made that is too big to fit into a chunk then it is allocated
 * directly using malloc.
 */
struct SVR_Arena_s {
    /**
     * The base address of this memory allocation
     */
    void* base;

    /**
     * The write index (in bytes) within the block
     */
    size_t write_index;

    /**
     * The block this allocation was made from or NULL if this block is
     * directly allocated using malloc
     */
    SVR_BlockAllocator* allocator;

    /**
     * Pointer to the next chunk in this arena
     */
    struct SVR_Arena_s* next;
};

/** \} */

void SVR_MemPool_init(void);
void SVR_MemPool_close(void);

SVR_Arena* SVR_Arena_alloc(SVR_BlockAllocator* allocator);
void* SVR_Arena_sprintf(SVR_Arena* alloc, const char* format, ...);
void* SVR_Arena_write(SVR_Arena* alloc, const void* data, size_t size);
void* SVR_Arena_strdup(SVR_Arena* alloc, const char* s);
void* SVR_Arena_reserve(SVR_Arena* alloc, size_t size);
void SVR_Arena_free(SVR_Arena* alloc);

#endif // #ifndef __SVR_ARENA_H
