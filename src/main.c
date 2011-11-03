
#include <svr.h>

/* Initialize core SVR components that are needed even when the client aspects of SVR are not in use */
void SVR_initCore(void) {
    SVR_Lockable_initMutexAttributes();
    SVR_RefCounter_init();
    SVR_BlockAlloc_init();
    SVR_MemPool_init();
    SVR_Message_init();
    SVR_Encoding_init();
}

void SVR_init(void) {
    SVR_initCore();

    SVR_Stream_init();
    SVR_Comm_init();
}
