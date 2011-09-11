
#ifndef __SVR_BLOCKALLOC_H
#define __SVR_BLOCKALLOC_H

#include <stdint.h>
#include <pthread.h>

#include <seawolf.h>

#include <svr/forward.h>

struct BlockAllocator_s {
    size_t block_size;
    size_t grow_size;
    size_t num_blocks;
    size_t index;
    List* chunks;
    void** blocks;
    pthread_mutex_t lock;
};

void BlockAlloc_init(void);
void BlockAlloc_close(void);
BlockAllocator* BlockAlloc_newAllocator(size_t block_size, size_t grow_size);
void BlockAlloc_freeAllocator(BlockAllocator* allocator);
BlockAllocator* BlockAlloc_getSharedAllocator(uint32_t block_size);
size_t BlockAlloc_getBlockSize(BlockAllocator* allocator);
void* BlockAlloc_alloc(BlockAllocator* allocator);
void BlockAlloc_free(BlockAllocator* allocator, void* p);

#endif // #ifndef __SVR_BLOCKALLOC_H
