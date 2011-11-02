
#include <svr.h>

#include "encoding_internal.h"

static void encode(SVR_Encoder* encoder, IplImage* frame);
static void decode(SVR_Decoder* decoder, void* data, size_t n);

SVR_Encoding SVR_ENCODING(raw) = {
        .name = "raw",
        .openEncoder = NULL,
        .closeEncoder = NULL,
        .encode = encode,
        .openDecoder = NULL,
        .closeDecoder = NULL,
        .decode = decode
};

typedef struct {
    int write_index;
} SVR_RawEncoding_Decoder;

static void encode(SVR_Encoder* encoder, IplImage* frame) {
    SVR_Encoder_provideData(encoder, frame->imageData, frame->imageSize);
}

static void decode(SVR_Decoder* decoder, void* data, size_t n) {
    SVR_Decoder_writePaddedFrameData(decoder, data, n);
}
