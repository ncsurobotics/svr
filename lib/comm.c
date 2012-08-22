/**
 * \file
 * \brief Server communications
 */

#include <svr.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

#define MAX_REQUEST_ID ((unsigned int)0xffff)

static int client_sock = -1;
static pthread_t receive_thread;
static SVR_ResponseSet* response_set;
static pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;
static void* payload_buffer = NULL;
static int payload_buffer_size = 0;

static void* SVR_Comm_receiveThread(void* _unused);

/**
 * \defgroup ServComm Communication management
 * \ingroup Comm
 * \brief Provides synchronous request-response communication with an SVR server
 * \{
 */

/**
 * \brief Initialize Comm module
 *
 * Initialize Comm module and connect to SVR server
 *
 * \param server_address IP address of the server as a string
 * \return 0 on success, -1 on failure
 */
int SVR_Comm_init(const char* server_address) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_address);
    addr.sin_port = htons(33560);

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock == -1) {
        SVR_log(SVR_ERROR, "Unable to create socket");
        return -1;
    }

    if(connect(client_sock, (struct sockaddr*) &addr, sizeof(addr))) {
        SVR_log(SVR_ERROR, "Unable to connect to SVR server");
        return -1;
    }

    response_set = SVR_ResponseSet_new(MAX_REQUEST_ID);

    /* Spawn background thread */
    pthread_create(&receive_thread, NULL, SVR_Comm_receiveThread, NULL);

    return 0;
}

/**
 * \brief Background message receive thread
 *
 * Background thread started by SVR_Comm_init and responsible for handling
 * incoming messages and message response
 *
 * \param _unused Unused
 * \return Always returns NULL
 */
static void* SVR_Comm_receiveThread(void* _unused) {
    SVR_Message* message;
    int n;

    while(true) {
        message = SVR_Net_receiveMessage(client_sock);

        if(message == NULL) {
            SVR_log(SVR_ERROR, "Server has closed");
            break;
        }

        /* Retrieve payload */
        if(message->payload_size) {
            if(message->payload_size > payload_buffer_size) {
                payload_buffer = realloc(payload_buffer, message->payload_size);
                payload_buffer_size = message->payload_size;
            }

            message->payload = payload_buffer;
            n = SVR_Net_receivePayload(client_sock, message);
            if(n <= 0) {
                SVR_log(SVR_ERROR, "Server has closed");
                break;
            }
        }

        if(message->request_id) {
            SVR_ResponseSet_setResponse(response_set, message->request_id - 1, message);
        } else {
            SVR_MessageRouter_processMessage(message);
            SVR_Message_release(message);
        }
    }

    return NULL;
}

/**
 * \brief Send a message
 *
 * Send a message. If the message is a request then a request ID will be
 * generated for it and this call will block until a response is
 * available. Otherwise, the message is sent and NULL returned. This call will
 * not release the message.
 *
 * \param message Message to send
 * \param is_request If true, then a request ID will be generated for the
 * message and this call will block until a response becomes available. If
 * false, then this function will return as soon as the message is sent.
 * \return The response to the message if is_request is true, or NULL otherwise.
 */
void* SVR_Comm_sendMessage(SVR_Message* message, bool is_request) {
    void* response = NULL;

    if(is_request) {
        message->request_id = SVR_ResponseSet_getRequestId(response_set) + 1;
    }

    pthread_mutex_lock(&send_lock);
    SVR_Net_sendMessage(client_sock, message);
    pthread_mutex_unlock(&send_lock);

    if(is_request) {
        response = SVR_ResponseSet_getResponse(response_set, message->request_id - 1);
    }

    return response;
}

/**
 * \brief Parse a SVR.response message
 *
 * Handle a SVR.response message and the associated return code
 *
 * \param response The response messages to verify
 * \return The contained error code, or -1 of the message can not be parsed as
 * an SVR.response message
 */
int SVR_Comm_parseResponse(SVR_Message* response) {
    if(response->count == 2 && strcmp(response->components[0], "SVR.response") == 0) {
        return atoi(response->components[1]);
    } else {
        return -1;
    }
}

/** \} */
