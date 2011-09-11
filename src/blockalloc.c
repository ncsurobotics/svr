
#include <svr.h>

static List* piles = NULL;
static pthread_mutex_t piles_lock = PTHREAD_MUTEX_INITIALIZER;

void BlockAlloc_init(void) {
    piles = List_new();
}

void BlockAlloc_close(void) {
    BlockAllocator* allocator;

    while((allocator = List_remove(piles, 0)) != NULL) {
        for(int i = 0; i < allocator->index; i++) {
            free(allocator->blocks[i]);
        }
        free(allocator);
    }
    
    List_destroy(piles);
}

BlockAllocator* BlockAlloc_getAllocator(uint32_t block_size) {
    BlockAllocator* allocator;
    int i = 0;

    pthread_mutex_lock(&piles_lock);
    while((allocator = List_get(piles, i)) != NULL) {
        if(allocator->block_size == block_size) {
            break;
        }
        i++;
    }

    if(allocator == NULL) {
        allocator = malloc(sizeof(BlockAllocator));
        allocator->block_size = block_size;
        allocator->grow_size = 4;
        allocator->num_blocks = 0;
        allocator->index = 0;
        allocator->blocks = NULL;
        pthread_mutex_init(&allocator->lock, NULL);

        List_append(piles, allocator);
    }
    pthread_mutex_unlock(&piles_lock);

    return allocator;
}

void* BlockAlloc_alloc(BlockAllocator* allocator) {
    void* p;

    pthread_mutex_lock(&allocator->lock);

    if(allocator->index == 0) {
        allocator->num_blocks += allocator->grow_size;
        allocator->blocks = realloc(allocator->blocks, allocator->num_blocks * sizeof(void*));
        allocator->index = allocator->grow_size;

        for(int i = 0; i < allocator->index; i++) {
            allocator->blocks[i] = malloc(allocator->block_size);
        }
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
