
#include "svr.h"

#define DEFAULT_GROW_SIZE 4

static List* shared_allocators = NULL;
static pthread_mutex_t piles_lock = PTHREAD_MUTEX_INITIALIZER;

/* TODO: Allocate blocks in chunks using one malloc. store chunk pointers in a list */

void BlockAlloc_init(void) {
    shared_allocators = List_new();
}

void BlockAlloc_close(void) {
    BlockAllocator* allocator;

    while ((allocator = List_remove(shared_allocators, 0)) != NULL) {
        BlockAlloc_freeAllocator(allocator);
    }

    List_destroy(shared_allocators);
}

BlockAllocator* BlockAlloc_newAllocator(size_t block_size, size_t grow_size) {
    BlockAllocator* allocator;

    allocator = malloc(sizeof(BlockAllocator));
    allocator->block_size = block_size;
    allocator->grow_size = grow_size;
    allocator->num_blocks = 0;
    allocator->index = 0;
    allocator->chunks = List_new();
    allocator->blocks = NULL;
    pthread_mutex_init(&allocator->lock, NULL);

    return allocator;
}

void BlockAlloc_freeAllocator(BlockAllocator* allocator) {
    void* chunk;

    while((chunk = List_get(allocator->chunks, 0)) != NULL) {
        free(chunk);
    }
    
    free(allocator->blocks);
    free(allocator);
}

BlockAllocator* BlockAlloc_getSharedAllocator(uint32_t block_size) {
    BlockAllocator* allocator;
    int i = 0;

    pthread_mutex_lock(&piles_lock);
    while ((allocator = List_get(shared_allocators, i)) != NULL) {
        if (allocator->block_size == block_size) {
            break;
        }
        i++;
    }

    if (allocator == NULL) {
        allocator = BlockAlloc_newAllocator(block_size, DEFAULT_GROW_SIZE);
        List_append(shared_allocators, allocator);
    }
    pthread_mutex_unlock(&piles_lock);

    return allocator;
}

size_t BlockAlloc_getBlockSize(BlockAllocator* allocator) {
    return allocator->block_size;
}

void* BlockAlloc_alloc(BlockAllocator* allocator) {
    void* p;

    pthread_mutex_lock(&allocator->lock);

    if (allocator->index == 0) {
        allocator->num_blocks += allocator->grow_size;
        allocator->blocks = realloc(allocator->blocks, allocator->num_blocks * sizeof(void*));
        allocator->index = allocator->grow_size;

        allocator->blocks[0] = malloc(allocator->block_size * allocator->grow_size);

        for(int i = 1; i < allocator->index; i++) {
            allocator->blocks[i] = ((uint8_t*)allocator->blocks[0]) + (i * allocator->block_size);
        }

        List_append(allocator->chunks, allocator->blocks[0]);
    }

    allocator->index -= 1;
    p = allocator->blocks[allocator->index];

    pthread_mutex_unlock(&allocator->lock);

    return p;
}

void BlockAlloc_free(BlockAllocator* allocator, void* p) {
    pthread_mutex_lock(&allocator->lock);
    allocator->blocks[allocator->index] = p;
    allocator->index++;
    pthread_mutex_unlock(&allocator->lock);
}
