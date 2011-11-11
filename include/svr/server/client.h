
#ifndef __SVR_SERVER_CLIENT_H
#define __SVR_SERVER_CLIENT_H

#include <seawolf.h>

#include <svr/forward.h>
#include <svr/server/forward.h>

/**
 * Client state
 */
typedef enum {
    /**
     * State is unknown or unset
     */
    SVR_UNKNOWN,

    /**
     * Client is connected, but unauthenticated
     */
    SVR_UNAUTHENTICATED,

    /**
     * Client is authenticated (full connected)
     */
    SVR_CONNECTED,

    /**
     * Client connection is closed
     */
    SVR_CLOSED
} SVRs_Client_State;

struct SVRs_Client_s {
    /**
     * Socket file descriptor
     */
    int socket;

    SVRs_Client_State state;

    /**
     * Client's thread
     */
    pthread_t thread;

    /**
     * List of streams client is subscribed to
     */
    Dictionary* streams;

    /**
     * List of sources the client provides
     */
    Dictionary* sources;

    void* payload_buffer;
    size_t payload_buffer_size;

    /* This object is reference counted */
    SVR_REFCOUNTED;

    /* This object is lockable */
    SVR_LOCKABLE;
};

void SVRs_Client_init(void);
void SVRs_Client_close(void);
SVRs_Client* SVRs_Client_new(int socket);
void SVRs_Client_provideSource(SVRs_Client* client, SVRs_Source* source);
void SVRs_Client_openStream(SVRs_Client* client, const char* stream_name);
void SVRs_Client_closeStream(SVRs_Client* client, const char* stream_name);
SVRs_Stream* SVRs_Client_getStream(SVRs_Client* client, const char* stream_name);
SVRs_Source* SVRs_Client_getSource(SVRs_Client* client, const char* source_name);
void SVRs_addClient(int socket);
void SVRs_Client_markForClosing(SVRs_Client* client);
void SVRs_Client_reply(SVRs_Client* client, SVR_Message* request, SVR_Message* response);
void SVRs_Client_replyCode(SVRs_Client* client, SVR_Message* request, int error_code);
void SVRs_Client_kick(SVRs_Client* client, const char* reason);
void SVRs_joinAllClientThreads(void);
List* SVRs_getAllClients(void);
void SVRs_acquireGlobalClientsLock(void);
void SVRs_releaseGlobalClientsLock(void);
int SVRs_Client_sendMessage(SVRs_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_CLIENT_H
