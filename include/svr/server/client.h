
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

    /* This object is reference counted */
    SVR_REFCOUNTED;

    /* This object is lockable */
    SVR_LOCKABLE;
};

void SVRs_Client_init(void);
void SVRs_Client_close(void);
SVRs_Client* SVRs_Client_new(int socket);
void SVRs_Client_markForClosing(SVRs_Client* client);
void SVRs_Client_kick(SVRs_Client* client, const char* reason);
int SVRs_Client_sendMessage(SVRs_Client* client, SVR_Message* message);

void SVRs_addClient(int socket);
List* SVRs_getAllClients(void);
void SVRs_joinAllClientThreads(void);
void SVRs_acquireGlobalClientsLock(void);
void SVRs_releaseGlobalClientsLock(void);

/**
 * \brief Write encoded stream data
 *
 * Once data has been encoded by the stream it is passed to the client to be
 * written out as a frame data packet
 */
size_t SVRs_Client_writeStreamData(SVRs_Client* client, SVRs_Stream* stream, SVR_DataBuffer* data, size_t data_available);

/**
 * Subscribe the client to the given source using the given encoding
 */
int SVRs_Client_openStream(SVRs_Client* client, char* source_name, char* encoding_name);

/**
 * Request the stream associated with the given source be resized
 */
int SVRs_Client_resizeStream(SVRs_Client* client, char* source_name, uint16_t height, uint16_t width);

int SVRs_Client_closeStream(SVRs_Client* client, char* source_name);

// void Client_newClientSource(Client* client, char* source_name,

#endif // #ifndef __SVR_SERVER_CLIENT_H
