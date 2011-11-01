
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

    SVR_Stream_State state;

    void* buffer;
    size_t buffer_size;

    char* name;

    SVR_LOCKABLE;
};

/**
 * Create a new stream with the given client, source, and encoding
 */
void SVRs_Stream_destroy(SVRs_Stream* stream);

/**
 * Called by the source to provide raw data to the stream
 */
void SVRs_Stream_inputSourceData(SVRs_Stream* stream, void* data, size_t data_available);

SVRs_Stream* SVRs_Stream_new(SVRs_Client* client, SVRs_Source* source, const char* name);
void SVRs_Stream_setEncoding(SVRs_Stream* stream, SVR_Encoding* encoding);
void SVRs_Stream_pause(SVRs_Stream* stream);
void SVRs_Stream_start(SVRs_Stream* stream);
void SVRs_Stream_close(SVRs_Stream* stream);



#endif // #ifndef __SVR_SERVER_STREAM_H
