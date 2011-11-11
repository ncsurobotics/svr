
#include <svr.h>

#include <stdlib.h>

static char* server_address = NULL;

/* Initialize core SVR components that are needed even when the client aspects of SVR are not in use */
void SVR_initCore(void) {
    SVR_Lockable_initMutexAttributes();
    SVR_RefCounter_init();
    SVR_BlockAlloc_init();
    SVR_MemPool_init();
    SVR_Message_init();
    SVR_Encoding_init();
}

void SVR_setServerAddress(char* address) {
    if(server_address) {
        free(server_address);
    }
    server_address = strdup(address);
}

int SVR_init(void) {
    SVR_initCore();

    if(getenv("SVR_DEBUG")) {
        SVR_Logging_setThreshold(SVR_DEBUG);
    }

    if(getenv("SVR_SERVER")) {
        SVR_setServerAddress(getenv("SVR_SERVER"));
        SVR_log(SVR_WARNING, Util_format("Using SVR server \"%s\" from SVR_SERVER environment variable", server_address));
    }

    SVR_Stream_init();

    if(server_address) {
        return SVR_Comm_init(server_address);
    } else {
        return SVR_Comm_init("127.0.0.1");
    }
}
