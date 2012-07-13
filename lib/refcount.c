/**
 * \file
 * \brief Reference counter
 */

#include <seawolf.h>

#include "svr.h"

static Queue* garbage_queue = NULL;
static SVR_BlockAllocator* allocator = NULL;
static pthread_t garbage_collector_thread;

static void* SVR_RefCounter_garbageCollector(void* __unused);

/**
 * \defgroup RefCounter Reference counter
 * \ingroup Util
 * \brief Reference counter based garbage collection
 *
 * A reference countable object can be defined by creating a structure
 * containing the SVR_REFCOUNTED member, e.g.
 *
 * \code
 * typedef struct {
 *   // ...
 *   SVR_REFCOUNTED;
 * } MyType;
 * \endcode
 *
 * When an instance of such an object is created, SVR_REFCOUNTED_INIT should be
 * called with the object and a cleanup routine as arguments,
 *
 * \code
 *   MyType* my_object = ...
 *   SVR_REFCOUNTED_INIT(my_object, my_cleanup_routine);
 *   ...
 * \endcode
 *
 * The initial reference count of an object is 1. The reference count is
 * incremented by calls to SVR_REF and decrement by calls to SVR_UNREF. Once the
 * reference count reachs 0, the object is considered unused and will be queued
 * for garbage collection. The garbage collector will call the cleanup routine
 * passed to SVR_REFCOUNTED_INIT with the object pass as the argument to the
 * cleanup routine.
 *
 * \{
 */

/**
 * \private
 * \brief Initialize reference counting module
 *
 * Allocate global data structures used by reference counters. This is called by
 * SVR_initCore.
 */
void SVR_RefCounter_init(void) {
    garbage_queue = Queue_new();
    allocator = SVR_BlockAlloc_newAllocator(sizeof(SVR_RefCounter), 8);

    pthread_create(&garbage_collector_thread, NULL, &SVR_RefCounter_garbageCollector, NULL);
}

/**
 * \private
 * \brief Close reference counting module
 *
 * Deallocate global data structures used by reference counters
 */
void SVR_RefCounter_close(void) {
    Queue_append(garbage_queue, NULL);
    SVR_BlockAlloc_freeAllocator(allocator);
}

/**
 * \private
 * \brief Allocate a new reference counter
 *
 * Initialize a new reference counter. A new reference counter initially has a
 * reference count of 1. This function should not be called directly, but
 * instead through SVR_REFCOUNTED_INIT.
 *
 * \param cleanup Cleanup routine for the given object. The cleanup routine is
 * passed the object as its argument
 * \param object The object the reference counter is to be associated with
 * \return A new reference counter object
 */
SVR_RefCounter* SVR_RefCounter_new(void (*cleanup)(void*), void* object) {
    SVR_RefCounter* ref_counter;

    ref_counter = SVR_BlockAlloc_alloc(allocator);
    ref_counter->ref_count = 1;
    ref_counter->cleanup = cleanup;
    ref_counter->object = object;
    SVR_LOCKABLE_INIT(ref_counter);

    return ref_counter;
}

/**
 * \private
 * \brief Free a reference counter
 *
 * Free a reference counter
 *
 * \param ref_counter Reference counter to free
 */
void SVR_RefCounter_destroy(SVR_RefCounter* ref_counter) {
    SVR_BlockAlloc_free(allocator, ref_counter);
}

/**
 * \private
 * \brief Garbage collector thread
 *
 * Background thread that cleans up objects and frees reference counters when
 * their reference count reaches 0. This is done asychronously from SVR_UNREF to
 * avoid any unexpected latency from object cleanup.
 *
 * \param __unused Unused parameter
 * \return Always returns NULL
 */
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

        SVR_RefCounter_destroy(ref_counter);
    }

    return NULL;
}

/**
 * \private
 * \brief Increment reference count
 *
 * Increment the reference count of the object. This function should not be
 * called directly, but through SVR_REF.
 *
 * \param ref_counter Reference counter to increment the reference count of
 */
void SVR_RefCounter_ref(SVR_RefCounter* ref_counter) {
    SVR_LOCK(ref_counter);
    ref_counter->ref_count++;
    SVR_UNLOCK(ref_counter);
}

/**
 * \private
 * \brief Decrement reference count
 *
 * Decrement the reference count of the object. An object will be marked for
 * garbage colleciton/cleanup once its reference count reaches 0. This function
 * should not be called directly, but through SVR_UNREF.
 *
 * \param ref_counter Reference counter to decrement the reference count of
 */
void SVR_RefCounter_unref(SVR_RefCounter* ref_counter) {
    SVR_LOCK(ref_counter);
    ref_counter->ref_count--;
    if(ref_counter->ref_count == 0) {
        Queue_append(garbage_queue, ref_counter);
    }
    SVR_UNLOCK(ref_counter);
}

/** \} */
