
#include "svr.h"
#include "svr/server/svr.h"

void SVRs_exit(void) {
    exit(0);
}

void SVRs_exitError(void) {
    exit(-1);
}

int main(void) {
    SVR_BlockAlloc_init();
    SVR_MemPool_init();
    SVR_Message_init();

    SVRs_Client_init();
    SVRs_Source_init();
    SVRs_Server_mainLoop();
    
    return 0;
}
