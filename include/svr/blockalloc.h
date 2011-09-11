
#ifndef __SVR_BLOCKALLOC_H
#define __SVR_BLOCKALLOC_H

#include <stdint.h>
#include <pthread.h>

#include <svr/forward.h>

struct BlockAllocator_s {
    uint32_t block_size;
    uint32_t grow_size;
    uint32_t num_blocks;
    uint32_t index;
    void** blocks;
    pthread_mutex_t lock;
};

void BlockAlloc_init(void);
void BlockAlloc_close(void);
BlockAllocator* BlockAlloc_getAllocator(uint32_t block_size);
void* BlockAlloc_alloc(BlockAllocator* allocator);
void BlockAlloc_free(BlockAllocator* allocator, void* p);

#endif // #ifndef __SVR_BLOCKALLOC_H
