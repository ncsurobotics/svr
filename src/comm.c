
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

static void* SVR_Comm_receiveThread(void* __unused);

int SVR_Comm_init(void) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
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

static void* SVR_Comm_receiveThread(void* __unused) {
    SVR_Message* message;

    while(true) {
        message = SVR_Net_receiveMessage(client_sock);

        /* Retrieve payload */
        if(message->payload_size) {
            if(message->payload_size > payload_buffer_size) {
                payload_buffer = realloc(payload_buffer, message->payload_size);
                payload_buffer_size = message->payload_size;
            }

            message->payload = payload_buffer;
            SVR_Net_receivePayload(client_sock, message);
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

int SVR_Comm_parseResponse(SVR_Message* response) {
    if(response->count == 2 && strcmp(response->components[0], "SVR.response") == 0) {
        return atoi(response->components[1]);
    } else {
        return -1;
    }
}
