
#ifndef __SVR_SERVER_CLIENT_H
#define __SVR_SERVER_CLIENT_H

#include <seawolf.h>

#include <svr/forward.h>
#include <svrd/forward.h>

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
} SVRD_Client_State;

struct SVRD_Client_s {
    /**
     * Socket file descriptor
     */
    int socket;

    SVRD_Client_State state;

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

void SVRD_Client_init(void);
void SVRD_Client_close(void);
SVRD_Client* SVRD_Client_new(int socket);
void SVRD_Client_provideSource(SVRD_Client* client, SVRD_Source* source);
void SVRD_Client_unprovideSource(SVRD_Client* client, SVRD_Source* source);
void SVRD_Client_openStream(SVRD_Client* client, const char* stream_name);
void SVRD_Client_closeStream(SVRD_Client* client, const char* stream_name);
SVRD_Stream* SVRD_Client_getStream(SVRD_Client* client, const char* stream_name);
SVRD_Source* SVRD_Client_getSource(SVRD_Client* client, const char* source_name);
void SVRD_addClient(int socket);
void SVRD_Client_markForClosing(SVRD_Client* client);
void SVRD_Client_reply(SVRD_Client* client, SVR_Message* request, SVR_Message* response);
void SVRD_Client_replyCode(SVRD_Client* client, SVR_Message* request, int error_code);
void SVRD_Client_kick(SVRD_Client* client, const char* reason);
void SVRD_joinAllClientThreads(void);
List* SVRD_getAllClients(void);
void SVRD_acquireGlobalClientsLock(void);
void SVRD_releaseGlobalClientsLock(void);
int SVRD_Client_sendMessage(SVRD_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_CLIENT_H
