
#define __SVR_LOCKABLE_C
#include <svr.h>

pthread_mutexattr_t __svr_lockable_recursive;

void SVR_Lockable_initMutexAttributes(void) {
    pthread_mutexattr_init(&__svr_lockable_recursive);
    pthread_mutexattr_settype(&__svr_lockable_recursive, PTHREAD_MUTEX_RECURSIVE);
}
