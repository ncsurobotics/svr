
#ifndef __SVR_SOURCE_H
#define __SVR_SOURCE_H

#include <svr/forward.h>

struct Source_s {
    Encoding* encoding;
    void* decoder_data;
    FrameProperties* frame_properties;
    List* streams;
};

/**
 * Register a stream with the given source
 */
void Source_registerStream(Source* source, Stream* stream);

/**
 * Give encoded data to the source to pass to its registered streams
 */
void Source_provideData(Source* source, DataBuffer* data, size_t data_available);

#endif // #ifndef __SVR_SOURCE_H
