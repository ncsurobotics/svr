
#ifndef __SVR_LOCKABLE_H
#define __SVR_LOCKABLE_H

#include <pthread.h>

#define SVR_LOCKABLE pthread_mutex_t __svr_lock
#define SVR_LOCKABLE_INIT(object) (pthread_mutex_init(&(object)->__svr_lock, NULL))
#define SVR_LOCK(object) (pthread_mutex_lock(&(object)->__svr_lock))
#define SVR_UNLOCK(object) (pthread_mutex_unlock(&(object)->__svr_lock))

#endif // #ifndef __SVR_LOCKABLE_H
