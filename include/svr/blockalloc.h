
#ifndef __SVR_BLOCKALLOC_H
#define __SVR_BLOCKALLOC_H

#include <stdint.h>
#include <pthread.h>

#include <seawolf.h>

#include <svr/forward.h>

struct SVR_BlockAllocator_s {
    size_t block_size;
    size_t grow_size;
    size_t num_blocks;
    size_t index;
    List* chunks;
    void** blocks;
    pthread_mutex_t lock;
};

void SVR_BlockAlloc_init(void);
void SVR_BlockAlloc_close(void);
SVR_BlockAllocator* SVR_BlockAlloc_newAllocator(size_t block_size, size_t grow_size);
void SVR_BlockAlloc_freeAllocator(SVR_BlockAllocator* allocator);
SVR_BlockAllocator* SVR_BlockAlloc_getSharedAllocator(uint32_t block_size);
size_t SVR_BlockAlloc_getBlockSize(SVR_BlockAllocator* allocator);
void* SVR_BlockAlloc_alloc(SVR_BlockAllocator* allocator);
void SVR_BlockAlloc_free(SVR_BlockAllocator* allocator, void* p);

#endif // #ifndef __SVR_BLOCKALLOC_H
