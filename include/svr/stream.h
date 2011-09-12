
#ifndef __SVR_STREAM_H
#define __SVR_STREAM_H

#include <svr/forward.h>

struct SVR_Stream_s {
    SVR_Client* client;
    SVR_Source* source;

    SVR_Encoding* encoding;

    void* encoder_data;

    /**
     * Takes raw data from a source and produces output data for the target encoding
     */
    SVR_Reencoder* reencoder;
    
    /**
     * Frame properties. Inherited from Source by default
     */
    SVR_FrameProperties* frame_properties;
};

/**
 * Create a new stream with the given client, source, and encoding
 */
void SVR_Stream_new(SVR_Client* client, SVR_Source* source, SVR_Encoding* encoding);

/**
 * Called by the source to provide raw data to the stream
 */
void SVR_Stream_inputSourceData(SVR_Stream* stream, SVR_DataBuffer* data, size_t data_available);

/**
 * Set the frame properties. Setting the frame properties will cause the stream
 * encoding and the reencoder to be reallocated which may cause brief frame
 * loss to the stream
 */
void SVR_Stream_setFrameProperties(SVR_Stream* stream, SVR_FrameProperties* frame_properties);

#endif // #ifndef __SVR_STREAM_H
