/**
 * \file
 * \brief Request processing loop
 */

#include "svr.h"
#include "svr/server/svr.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

/** Server socket */
static int svr_sock = -1;

/** Server socket bind address */
struct sockaddr_in svr_addr;

/** Flag to keep SVR_mainLoop running */
static bool run_mainloop = true;

/** Flag used by SVR_mainLoop to signal its completion */
static bool mainloop_running = false;

/** Condition to wait for when waiting for SVR_mainLoop to complete */
static pthread_cond_t mainloop_done = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mainloop_done_lock = PTHREAD_MUTEX_INITIALIZER;

static void SVRs_Server_initServerSocket(void);

/**
 * \defgroup netloop Net loop
 * \brief Main hub request loop and support routines
 * \{
 */

/**
 * \brief Initialize the sever socket
 *
 * Perform all required initialization of the server socket so that it is
 * prepared to being accepting connections
 */
static void SVRs_Server_initServerSocket(void) {
    /* Used to set the SO_REUSEADDR socket option on the server socket */
    const int reuse = 1;

    /* Initialize the connection structure to bind the the correct port on all
       interfaces */
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    svr_addr.sin_port = htons(33560);

    /* Create the socket */
    svr_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(svr_sock == -1) {
        SVR_log(SVR_CRITICAL, Util_format("Error creating socket: %s", strerror(errno)));
        SVRs_exitError();
    }

    /* Allow localhost address reuse. This allows us to restart the hub after it
       unexpectedly dies and leaves a stale socket */
    setsockopt(svr_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /* Bind the socket to the server port/address */
    if(bind(svr_sock, (struct sockaddr*) &svr_addr, sizeof(svr_addr)) == -1) {
        SVR_log(SVR_CRITICAL, Util_format("Error binding socket: %s", strerror(errno)));
        SVRs_exitError();
    }

    /* Start listening */
    if(listen(svr_sock, MAX_CLIENTS)) {
        SVR_log(SVR_CRITICAL, Util_format("Error setting socket to listen: %s", strerror(errno)));
        SVRs_exitError();
    }
}

/**
 * \brief Perform sychronous pre-shutdown for signal handlers
 *
 * Perform synchronous pre-shutdown for signal handlers. When a signal is
 * recieved and goes unprocessed, SVR_mainLoop will consider this an error
 * condition. Signal handlers can call this before starting a asychronous
 * shutdown to avoid this issue.
 */
void SVRs_Server_preClose(void) {
    /* Set main loop to terminate */
    run_mainloop = false;
}

/**
 * \brief Close the net subsystem
 *
 * Perform a controlled shutdown of the net subsystem. Free all associated
 * memory and properly shutdown all associated sockets
 */
void SVRs_Server_close(void) {
    /* Synchronize exit with mainLoop */
    pthread_mutex_lock(&mainloop_done_lock);

    if(mainloop_running) {
        /* Instruct mainLoop to terminate */
        SVRs_Server_preClose();

        /* Now wait for SVR_mainLoop to terminate */
        while(mainloop_running) {
            pthread_cond_wait(&mainloop_done, &mainloop_done_lock);
        }
    }

    pthread_mutex_unlock(&mainloop_done_lock);
}

/**
 * \brief SVR main loop
 *
 * Main loop which processes client requests and handles all client connections
 */
void SVRs_Server_mainLoop(void) {
    /* Temporary storage for new client connections until a SVR_Client structure
       can be allocated for them */
    int client_new = 0;

    /* Create and ready the server socket */
    SVRs_Server_initServerSocket();

    /* Begin accepting connections */
    SVR_log(SVR_INFO, "Accepting client connections");

    /* Main loop is now running */
    mainloop_running = true;

    /* Start sending/recieving messages */
    while(run_mainloop) {
        client_new = accept(svr_sock, NULL, 0);

        if(run_mainloop == false ) {
            break;
        }

        if(client_new < 0) {
            SVR_log(SVR_ERROR, "Error accepting new client connection");
            continue;
        }

        SVRs_addClient(client_new);
    }

    /* Signal loop as ended */
    pthread_mutex_lock(&mainloop_done_lock);
    mainloop_running = false;
    shutdown(svr_sock, SHUT_RDWR);
    close(svr_sock);

    pthread_cond_broadcast(&mainloop_done);
    pthread_mutex_unlock(&mainloop_done_lock);
}
/** \} */
