
#ifndef __SVR_SERVER_CLIENT_H
#define __SVR_SERVER_CLIENT_H

#include <seawolf.h>

#include <svr/forward.h>

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
} SVR_Client_State;

struct SVR_Client_s {
    /**
     * Socket file descriptor
     */
    int socket;

    SVR_Client_State state;

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

void SVR_Client_init(void);
void SVR_Client_close(void);
SVR_Client* SVR_Client_new(int socket);
void SVR_Client_markForClosing(SVR_Client* client);
void SVR_Client_kick(SVR_Client* client, const char* reason);

void SVR_addClient(int socket);
List* SVR_getAllClients(void);
void SVR_joinAllClientThreads(void);
void SVR_acquireGlobalClientsLock(void);
void SVR_releaseGlobalClientsLock(void);

/**
 * \brief Write encoded stream data
 *
 * Once data has been encoded by the stream it is passed to the client to be
 * written out as a frame data packet
 */
size_t SVR_Client_writeStreamData(SVR_Client* client, SVR_Stream* stream, SVR_DataBuffer* data, size_t data_available);

/**
 * Subscribe the client to the given source using the given encoding
 */
int SVR_Client_openStream(SVR_Client* client, char* source_name, char* encoding_name);

/**
 * Request the stream associated with the given source be resized
 */
int SVR_Client_resizeStream(SVR_Client* client, char* source_name, uint16_t height, uint16_t width);

int SVR_Client_closeStream(SVR_Client* client, char* source_name);

// void Client_newClientSource(Client* client, char* source_name,

#endif // #ifndef __SVR_SERVER_CLIENT_H
