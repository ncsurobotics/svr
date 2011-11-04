
#include "svr.h"
#include "svr/server/svr.h"

#include <signal.h>

void SVRs_exit(void) {
    exit(0);
}

void SVRs_exitError(void) {
    exit(-1);
}

int main(void) {
    SVR_initCore();

    SVRs_Client_init();
    SVRs_Source_init();
    SVRs_MessageRouter_init();

    /* Spawn the test source */
    TestSource_open();

    /* Please *ignore* SIGPIPE. It will cause the program to close in the case
       of writing to a closed socket. We handle this ourselves. */
    signal(SIGPIPE, SIG_IGN);

    SVRs_Server_mainLoop();
    
    return 0;
}
