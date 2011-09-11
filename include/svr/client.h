
#ifndef __SVR_CLIENT_H
#define __SVR_CLIENT_H

#include <svr/forward.h>

struct Client_s {
    int socket;

    /**
     * List of streams client is subscribed to
     */
    Dictionary* streams;

    /**
     * List of sources the client provides
     */
    Dictionary* sources;
};

/**
 * \brief Write encoded stream data
 *
 * Once data has been encoded by the stream it is passed to the client to be
 * written out as a frame data packet
 */
size_t Client_writeStreamData(Client* client, Stream* stream, DataBuffer* data, size_t data_available);

/**
 * Subscribe the client to the given source using the given encoding
 */
int Client_openStream(Client* client, char* source_name, char* encoding_name);

/**
 * Request the stream associated with the given source be resized
 */
int Client_resizeStream(Client* client, char* source_name, uint16_t height, uint16_t width);

int Client_closeStream(Client* client, char* source_name);

//void Client_newClientSource(Client* client, char* source_name, 

#endif // #ifndef __SVR_CLIENT_H
