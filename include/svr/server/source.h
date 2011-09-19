
#ifndef __SVR_SERVER_SOURCE_H
#define __SVR_SERVER_SOURCE_H

#include <svr/forward.h>

struct SVRs_Source_s {
    SVR_Encoding* encoding;
    void* decoder_data;
    SVR_FrameProperties* frame_properties;
    List* streams;
};

/**
 * Register a stream with the given source
 */
void SVRs_Source_registerStream(SVRs_Source* source, SVRs_Stream* stream);

/**
 * Give encoded data to the source to pass to its registered streams
 */
void SVRs_Source_provideData(SVRs_Source* source, SVR_DataBuffer* data, size_t data_available);

void SVRs_Source_open(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_close(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_getProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_setProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_data(SVRs_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_SOURCE_H

