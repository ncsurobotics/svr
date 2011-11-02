
#ifndef __SVR_ENCODING_H
#define __SVR_ENCODING_H

#include <stdint.h>
#include <stdlib.h>

#include <svr/forward.h>
#include <svr/cv.h>

struct SVR_Encoding_s {
    /** 
     * The name of the encoding
     */
    const char* name;

    /**
     * Create a new encoder instance. The returned data is the
     * private data of the encoder
     */
    void* (*openEncoder)(SVR_FrameProperties* frame_properties);

    /**
     * Create a new decoder instance. The returned data is the
     * private data of the decoder
     */
    void* (*openDecoder)(SVR_FrameProperties* frame_properties);

    /**
     * Destroy and close the encoder instance
     */
    void (*closeEncoder)(SVR_Encoder* encoder);

    /**
     * Destroy and close the decoder instance
     */
    void (*closeDecoder)(SVR_Decoder* decoder);

    /**
     * Encode a frame. Return number of bytes ready to be read
     */
    void (*encode)(SVR_Encoder* encoder, IplImage* frame);

    /**
     * Provide data for decoding. Return number of frames ready
     */
    void (*decode)(SVR_Decoder* decoder, void* data, size_t n);
};

struct SVR_Encoder_s {
    void* buffer;
    unsigned int read_index;
    unsigned int write_index;
    ssize_t buffer_size;

    SVR_Encoding* encoding;
    void* private_data;
    SVR_LOCKABLE;
};

struct SVR_Decoder_s {
    List* ready_frames;
    List* free_frames;
    unsigned int write_offset;

    SVR_FrameProperties* frame_properties;
    SVR_Encoding* encoding;
    void* private_data;
    SVR_LOCKABLE;
};

#define SVR_ENCODING(name) __svr_##name##_encoding

void SVR_Encoding_init(void);
void SVR_Encoding_close(void);

SVR_Encoding* SVR_Encoding_getByName(const char* name);
int SVR_Encoding_register(SVR_Encoding* encoding);

SVR_Encoder* SVR_Encoder_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties);
void SVR_Encoder_destroy(SVR_Encoder* encoder);
size_t SVR_Encoder_encode(SVR_Encoder* encoder, IplImage* frame);
size_t SVR_Encoder_dataReady(SVR_Encoder* encoder);
size_t SVR_Encoder_readData(SVR_Encoder* encoder, void* buffer, size_t buffer_size);

SVR_Decoder* SVR_Decoder_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties);
void SVR_Decoder_destroy(SVR_Decoder* decoder);
int SVR_Decoder_decode(SVR_Decoder* decoder, void* data, size_t n);
int SVR_Decoder_framesReady(SVR_Decoder* decoder);
IplImage* SVR_Decoder_getFrame(SVR_Decoder* decoder);
void SVR_Decoder_returnFrame(SVR_Decoder* decoder, IplImage* frame);

#endif // #ifndef __SVR_ENCODING_H



