
#ifndef __SVR_STREAM_H
#define __SVR_STREAM_H

#include <svr/forward.h>

struct Stream_s {
    Client* client;
    Source* source;

    Encoding* encoding;

    void* encoder_data;

    /**
     * Takes raw data from a source and produces output data for the target encoding
     */
    Reencoder* reencoder;
    
    /**
     * Frame properties. Inherited from Source by default
     */
    FrameProperties* frame_properties;
};

/**
 * Create a new stream with the given client, source, and encoding
 */
void Stream_new(Client* client, Source* source, Encoding* encoding);

/**
 * Called by the source to provide raw data to the stream
 */
void Stream_inputSourceData(Stream* stream, DataBuffer* data, size_t data_available);

/**
 * Set the frame properties. Setting the frame properties will cause the stream
 * encoding and the reencoder to be reallocated which may cause brief frame
 * loss to the stream
 */
void Stream_setFrameProperties(Stream* stream, FrameProperties* frame_properties);

#endif // #ifndef __SVR_STREAM_H
