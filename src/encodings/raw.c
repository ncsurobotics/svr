
#include <svr.h>

#include "encoding_internal.h"

static void* openDecoder(SVR_FrameProperties* frame_properties);
static void closeDecoder(SVR_Decoder* decoder);
static void encode(SVR_Encoder* encoder, IplImage* frame);
static void decode(SVR_Decoder* decoder, void* data, size_t n);

SVR_Encoding SVR_ENCODING(raw) = {
        .name = "raw",
        .openEncoder = NULL,
        .closeEncoder = NULL,
        .encode = encode,
        .openDecoder = openDecoder,
        .closeDecoder = closeDecoder,
        .decode = decode
};

typedef struct {
    int write_index;
} SVR_RawEncoding_Decoder;

static void* openDecoder(SVR_FrameProperties* frame_properties) {
    SVR_RawEncoding_Decoder* decoder = malloc(sizeof(SVR_RawEncoding_Decoder));
    decoder->write_index = 0;
    return decoder;
}

static void closeDecoder(SVR_Decoder* decoder) {
    free(decoder->private_data);
}

static void encode(SVR_Encoder* encoder, IplImage* frame) {
    SVR_Encoder_provideData(encoder, frame->imageData, frame->imageSize);
}

static void decode(SVR_Decoder* decoder, void* data, size_t n) {
    SVR_RawEncoding_Decoder* private_data = decoder->private_data;
    IplImage* current_frame;
    size_t image_size;
    size_t chunk_size;
    int offset = 0;

    while(offset < n) {
        current_frame = SVR_Decoder_getCurrentFrame(decoder);
        image_size = current_frame->imageSize;

        chunk_size = Util_min(image_size - private_data->write_index, n - offset);
        memcpy(current_frame->imageData + private_data->write_index, ((uint8_t*)data) + offset, chunk_size);
        private_data->write_index = (private_data->write_index + chunk_size) % image_size;

        if(private_data->write_index == 0) {
            SVR_Decoder_currentFrameComplete(decoder);
        }
    }
}
