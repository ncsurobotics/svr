
#include <seawolf.h>

#include "svr.h"

static Queue* garbage_queue = NULL;
static SVR_BlockAllocator* allocator = NULL;
static pthread_t garbage_collector_thread;

static void* SVR_RefCounter_garbageCollector(void* __unused);

void SVR_RefCounter_init(void) {
    garbage_queue = Queue_new();
    allocator = SVR_BlockAlloc_newAllocator(sizeof(SVR_RefCounter), 8);

    pthread_create(&garbage_collector_thread, NULL, &SVR_RefCounter_garbageCollector, NULL);
}

void SVR_RefCounter_close(void) {
    Queue_append(garbage_queue, NULL);
    SVR_BlockAlloc_freeAllocator(allocator);
}

SVR_RefCounter* SVR_RefCounter_new(void (*cleanup)(void*), void* object) {
    SVR_RefCounter* ref_counter;

    ref_counter = SVR_BlockAlloc_alloc(allocator);
    ref_counter->ref_count = 1;
    ref_counter->cleanup = cleanup;
    ref_counter->object = object;
    SVR_LOCKABLE_INIT(ref_counter);
    
    return ref_counter;
}

void SVR_RefCounter_free(SVR_RefCounter* ref_counter) {
    SVR_BlockAlloc_free(allocator, ref_counter);
}

static void* SVR_RefCounter_garbageCollector(void* __unused) {
    SVR_RefCounter* ref_counter;

    while(true) {
        ref_counter = Queue_pop(garbage_queue, true);

        if(ref_counter == NULL) {
            break;
        }

        SVR_LOCK(ref_counter);
        ref_counter->cleanup(ref_counter->object);
        SVR_UNLOCK(ref_counter);

        SVR_RefCounter_free(ref_counter);
    }

    return NULL;
}

void SVR_RefCounter_ref(SVR_RefCounter* ref_counter) {
    SVR_LOCK(ref_counter);
    ref_counter->ref_count++;
    SVR_UNLOCK(ref_counter);
}

void SVR_RefCounter_unref(SVR_RefCounter* ref_counter) {
    SVR_LOCK(ref_counter);
    ref_counter->ref_count--;
    if(ref_counter->ref_count == 0) {
        Queue_append(garbage_queue, ref_counter);
    }
    SVR_UNLOCK(ref_counter);
}
