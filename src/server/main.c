
#include "svr.h"
#include "svr/server/svr.h"

void SVR_exit(void) {
    exit(0);
}

void SVR_exitError(void) {
    exit(-1);
}

int main(void) {
    SVR_BlockAlloc_init();
    SVR_MemPool_init();
    SVR_Message_init();

    SVR_Client_init();
    SVR_Server_mainLoop();
    
    return 0;
}
