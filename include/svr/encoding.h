
#ifndef __SVR_ENCODING_H
#define __SVR_ENCODING_H

#include <stdint.h>
#include <cv.h>

typedef struct Encoding_s {
    /**
     * Create a new encoder instance. The returned data represents the
     * private data of the encoder
     */
    void* (*openEncoder)(FrameProperties* frame_properties);

    /**
     * Create a new decoder instance. The returned data represents the
     * private data of the decoder
     */
    void* (*openDecoder)(FrameProperties* frame_properties);

    /**
     * Encode a frame. Possibly writes some output bytes to the provided buffer
     */
    ssize_t (*encode)(void* state, IplImage* frame, uint8_t* stream_buffer);

    /**
     * Provide data for decoding. Return a frame if one is ready. Otherwise,
     * return NULL. A returned IplImage may be clobbered by future calls to
     * decode.
     */
    IplImage* (*decode)(void* state, uint8_t* data, ssize_t n);

    /**
     * The name of the encoding
     */
    const char* name;
} Encoding;

Encoding* SVR_getEncoding(const char* name);

#endif // #ifndef __SVR_ENCODING_H
