
#ifndef __SVR_SERVER_SOURCE_H
#define __SVR_SERVER_SOURCE_H

#include <svr/forward.h>

struct SVR_Source_s {
    SVR_Encoding* encoding;
    void* decoder_data;
    SVR_FrameProperties* frame_properties;
    List* streams;
};

/**
 * Register a stream with the given source
 */
void SVR_Source_registerStream(SVR_Source* source, SVR_Stream* stream);

/**
 * Give encoded data to the source to pass to its registered streams
 */
void SVR_Source_provideData(SVR_Source* source, SVR_DataBuffer* data, size_t data_available);

#endif // #ifndef __SVR_SERVER_SOURCE_H

