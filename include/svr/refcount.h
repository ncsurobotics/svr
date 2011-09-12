
#ifndef __SVR_REFCOUNT_H
#define __SVR_REFCOUNT_H

struct SVR_RefCounter_s {
    uint32_t ref_count;
    pthread_mutex_t lock;
    void (*cleanup)(void*);
    void* object;
};

#define SVR_REF(object) (SVR_RefCounter_ref((object)->ref_counter, (object)))
#define SVR_UNREF(object) (SVR_RefCounter_unref((object)->ref_counter), (object))
#define SVR_REFCOUNTED SVR_RefCounter* ref_counter
#define SVR_REFCOUNTED_INIT(object, cleanup) ((object)->ref_counter = SVR_RefCounter_new((cleanup), (object)))

void SVR_RefCounter_init(void);
void SVR_RefCounter_close(void);
SVR_RefCounter* SVR_RefCounter_new(void (*cleanup)(void*), void* object);
void SVR_RefCounter_free(SVR_RefCounter* ref_counter);
void SVR_RefCounter_ref(SVR_RefCounter* ref_counter);
void SVR_RefCounter_unref(SVR_RefCounter* ref_counter);

#endif // #ifndef __SVR_REFCOUNT_H
