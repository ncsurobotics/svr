/**
 * \file
 * \brief Lockable
 */

#define __SVR_LOCKABLE_C
#include <svr.h>

/**
 * Recursive mutex attribute used to initialize all SVR_LOCKABLE object mutexes
 */
pthread_mutexattr_t __svr_lockable_recursive;

/**
 * \defgroup Lockable Lockable
 * \ingroup Util
 * \brief Lockable structure attribute
 *
 * A structure can be declared lockable by defining a member of the structure using SVR_LOCKABLE, e.g.
 *
 * \code
 * typedef struct {
 *   // ...
 *   SVR_LOCKABLE;
 * } MyType;
 * \endcode
 *
 * When an instance of such an object is created, SVR_LOCKABLE_INIT should be
 * called with the object as the argument,
 *
 * \code
 *   MyType* my_object = ...
 *   SVR_LOCKABLE_INIT(my_object);
 *   ...
 * \endcode
 *
 * A lock can be acquired for a lockable object by calling SVR_LOCK. Conversely,
 * an object can be unlocked by a call to SVR_UNLOCK.
 *
 * \{
 */

/**
 * \private
 * \brief Initialize global data structures shared by lockable objects
 *
 * Initialize global data structures shared by lockable objects
 */
void SVR_Lockable_initMutexAttributes(void) {
    pthread_mutexattr_init(&__svr_lockable_recursive);
    pthread_mutexattr_settype(&__svr_lockable_recursive, PTHREAD_MUTEX_RECURSIVE);
}

/** \} */
