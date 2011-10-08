
#ifndef __SVR_ENCODING_H
#define __SVR_ENCODING_H

typedef int IplImage;

#include <stdint.h>
#include <stdlib.h>
#include <svr/forward.h>

//#include <cv.h>

struct SVR_Encoding_s {
    /** 
     * The name of the encoding
     */
    const char* name;

    /**
     * Create a new encoder instance. The returned data represents the
     * private data of the encoder
     */
    void* (*openEncoder)(SVR_FrameProperties* frame_properties);

    /**
     * Create a new decoder instance. The returned data represents the
     * private data of the decoder
     */
    void* (*openDecoder)(SVR_FrameProperties* frame_properties);

    /**
     * Destroy and close the encoder instanance
     */
    void* (*closeEncoder)(void* private_data);

    /**
     * Destroy and close the decoder instance
     */
    void* (*closeDecoder)(void* private_data);
    
    /**
     * Create a buffer that should hold at least a frame's worth of encoded data
     */
    void* (*newOutputBuffer)(void* private_data, size_t* buffer_size);

    /**
     * Encode a frame. Possibly writes some output bytes to the provided buffer
     */
    ssize_t (*encode)(void* private_data, IplImage* frame, uint8_t* stream_buffer, size_t buffer_size);

    /**
     * Provide data for decoding. Return a frame if one is ready. Otherwise,
     * return NULL. A returned IplImage may be clobbered by future calls to
     * decode.
     */
    IplImage* (*decode)(void* private_data, uint8_t* data, ssize_t n);
};

SVR_Encoding* SVR_getEncoding(const char* name);

#endif // #ifndef __SVR_ENCODING_H




