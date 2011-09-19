
#ifndef __SVR_SERVER_STREAM_H
#define __SVR_SERVER_STREAM_H

#include <svr/forward.h>
#include <svr/server/forward.h>

struct SVRs_Stream_s {
    SVRs_Client* client;
    SVRs_Source* source;

    SVR_Encoding* encoding;

    void* encoder_data;

    /**
     * Takes raw data from a source and produces output data for the target encoding
     */
    SVRs_Reencoder* reencoder;
    
    /**
     * Frame properties. Inherited from Source by default
     */
    SVR_FrameProperties* frame_properties;
};

/**
 * Create a new stream with the given client, source, and encoding
 */
void SVRs_Stream_new(SVRs_Client* client, SVRs_Source* source, SVR_Encoding* encoding);

/**
 * Called by the source to provide raw data to the stream
 */
void SVRs_Stream_inputSourceData(SVRs_Stream* stream, SVR_DataBuffer* data, size_t data_available);

/**
 * Set the frame properties. Setting the frame properties will cause the stream
 * encoding and the reencoder to be reallocated which may cause brief frame
 * loss to the stream
 */
void SVRs_Stream_setFrameProperties(SVRs_Stream* stream, SVR_FrameProperties* frame_properties);

void SVRs_Stream_open(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_close(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_getProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_setProp(SVRs_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_STREAM_H
