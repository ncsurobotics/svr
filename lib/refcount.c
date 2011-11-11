
#include <seawolf.h>

#include "svr.h"

static Queue* garbage_queue = NULL;
static pthread_t garbage_collector_thread;

static void* SVR_RefCounter_garbageCollector(void* __unused);

void SVR_RefCounter_init(void) {
    garbage_queue = Queue_new();
    pthread_create(&garbage_collector_thread, NULL, &SVR_RefCounter_garbageCollector, NULL);
}

void SVR_RefCounter_close(void) {
    Queue_append(garbage_queue, NULL);
}

SVR_RefCounter* SVR_RefCounter_new(void (*cleanup)(void*), void* object) {
    SVR_RefCounter* ref_counter = malloc(sizeof(SVR_RefCounter));

    ref_counter->ref_count = 1;
    ref_counter->cleanup = cleanup;
    ref_counter->object = object;
    
    pthread_mutex_init(&ref_counter->lock, NULL);
    
    return ref_counter;
}

void SVR_RefCounter_free(SVR_RefCounter* ref_counter) {
    free(ref_counter);
}

static void* SVR_RefCounter_garbageCollector(void* __unused) {
    SVR_RefCounter* ref_counter;

    while(true) {
        ref_counter = Queue_pop(garbage_queue, true);

        if(ref_counter == NULL) {
            break;
        }

        ref_counter->cleanup(ref_counter->object);
        SVR_RefCounter_free(ref_counter);
    }

    return NULL;
}

void SVR_RefCounter_ref(SVR_RefCounter* ref_counter) {
    pthread_mutex_lock(&ref_counter->lock);
    ref_counter->ref_count++;
    pthread_mutex_unlock(&ref_counter->lock);
}

void SVR_RefCounter_unref(SVR_RefCounter* ref_counter) {
    pthread_mutex_lock(&ref_counter->lock);
    ref_counter->ref_count--;
    if(ref_counter->ref_count == 0) {
        Queue_append(garbage_queue, ref_counter);
    }
    pthread_mutex_unlock(&ref_counter->lock);
}
