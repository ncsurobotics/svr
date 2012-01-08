/**
 * \file
 * \brief Top-level library routines and initalization
 */

#include <svr.h>

#include <stdlib.h>

static char* server_address = NULL;

/**
 * \defgroup Main Main
 * \brief Top-level routines and initialization
 * \{
 */

/**
 * \brief Initialize core SVR components
 *
 * Initialize core SVR components used by both the client and server
 */
void SVR_initCore(void) {
    SVR_Lockable_initMutexAttributes();
    SVR_RefCounter_init();
    SVR_BlockAlloc_init();
    SVR_MemPool_init();
    SVR_Message_init();
    SVR_Encoding_init();
}

/**
 * \brief Set the SVR server address
 *
 * Specify the IP address of the SVR server to use. This must be called before
 * SVR_init
 *
 * \param address Address of the server as a string
 */
void SVR_setServerAddress(char* address) {
    if(server_address) {
        free(server_address);
    }
    server_address = strdup(address);
}

/**
 * \brief Initialize SVR
 *
 * Initialize all SVR components and connect to the SVR server
 *
 * \return 0 on success, all other values represent an error during
 * initialization
 */
int SVR_init(void) {
    SVR_initCore();

    if(getenv("SVR_DEBUG")) {
        SVR_Logging_setThreshold(SVR_DEBUG);
    }

    if(getenv("SVR_SERVER")) {
        SVR_setServerAddress(getenv("SVR_SERVER"));
        SVR_log(SVR_NORMAL, Util_format("Using SVR server \"%s\" from SVR_SERVER environment variable", server_address));
    }

    SVR_Stream_init();

    if(server_address) {
        return SVR_Comm_init(server_address);
    } else {
        return SVR_Comm_init("127.0.0.1");
    }
}

/** \} */
