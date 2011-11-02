
#include <svr.h>
#include <seawolf.h>

#include "encodings/encoding_internal.h"
#include "encodings/encodings.h"

static void SVR_Encoding_registerDefaultEncodings(void);

static Dictionary* encodings = NULL;

void SVR_Encoding_init(void) {
    encodings = Dictionary_new();
    SVR_Encoding_registerDefaultEncodings();
}

static void SVR_Encoding_registerDefaultEncodings(void) {
    SVR_Encoding_register(&SVR_ENCODING(raw));
}

void SVR_Encoding_close(void) {
    Dictionary_destroy(encodings);
}

SVR_Encoding* SVR_Encoding_getByName(const char* name) {
    return Dictionary_get(encodings, name);
}

int SVR_Encoding_register(SVR_Encoding* encoding) {
    if(Dictionary_exists(encodings, encoding->name)) {
        return -1;
    }

    Dictionary_set(encodings, encoding->name, encoding);

    return 0;
}

SVR_Encoder* SVR_Encoder_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties) {
    SVR_Encoder* encoder = malloc(sizeof(SVR_Encoder));

    encoder->encoding = encoding;
    encoder->buffer = NULL;
    encoder->write_index = 0;
    encoder->read_index = 0;
    encoder->buffer_size = 0;
    SVR_LOCKABLE_INIT(encoder);

    if(encoding->openEncoder) {
        encoder->private_data = encoding->openEncoder(frame_properties);
    } else {
        encoder->private_data = NULL;
    }

    return encoder;
}

void SVR_Encoder_destroy(SVR_Encoder* encoder) {
    if(encoder->encoding->closeEncoder) {
        encoder->encoding->closeEncoder(encoder);
    }

    free(encoder);
}

size_t SVR_Encoder_encode(SVR_Encoder* encoder, IplImage* frame) {
    encoder->encoding->encode(encoder, frame);
    return SVR_Encoder_dataReady(encoder);
}

size_t SVR_Encoder_dataReady(SVR_Encoder* encoder) {
    if(encoder->read_index <= encoder->write_index) {
        return encoder->write_index - encoder->read_index;
    } else {
        return (encoder->buffer_size - encoder->read_index) + encoder->write_index;
    }
}

size_t SVR_Encoder_readData(SVR_Encoder* encoder, void* buffer, size_t buffer_size) {
    size_t read_size;
    size_t first_chunk;

    SVR_LOCK(encoder);
    read_size = Util_min(SVR_Encoder_dataReady(encoder), buffer_size);
    first_chunk = Util_min(read_size, encoder->buffer_size - encoder->read_index + 1);

    memcpy(buffer, ((uint8_t*)encoder->buffer) + encoder->read_index, first_chunk);
    memcpy(((uint8_t*)buffer) + first_chunk, ((uint8_t*)encoder->buffer), read_size - first_chunk);
    encoder->read_index = (encoder->read_index + read_size) % encoder->buffer_size;
    SVR_UNLOCK(encoder);

    return read_size;
}

void SVR_Encoder_provideData(SVR_Encoder* encoder, void* data, size_t n) {
    size_t used_space;
    size_t free_space;
    size_t end_space;
    void* temp_space;

    SVR_LOCK(encoder);
    used_space = SVR_Encoder_dataReady(encoder);
    free_space = encoder->buffer_size - used_space;

    if(free_space < n) {
        if(encoder->write_index < encoder->read_index) {
            /* Temporarily store unread data */
            temp_space = malloc(used_space);
            SVR_Encoder_readData(encoder, temp_space, used_space);

            /* Grow buffer and restore data */
            encoder->buffer = realloc(encoder->buffer, used_space + n);
            memcpy(encoder->buffer, temp_space, used_space);
            encoder->read_index = 0;
            free(temp_space);

            encoder->write_index = used_space;
        } else{
            encoder->buffer = realloc(encoder->buffer, used_space + n);
        }

        encoder->buffer_size = used_space + n;
    }

    end_space = encoder->buffer_size - encoder->write_index;
    if(n <= end_space) {
        memcpy(((uint8_t*)encoder->buffer) + encoder->write_index, data, n);
    } else {
        memcpy(((uint8_t*)encoder->buffer) + encoder->write_index, data, end_space);
        memcpy(encoder->buffer, ((uint8_t*)data) + end_space, n - end_space);
    }
    encoder->write_index = (encoder->write_index + n) % encoder->buffer_size;

    SVR_UNLOCK(encoder);
}

SVR_Decoder* SVR_Decoder_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties) {
    SVR_Decoder* decoder = malloc(sizeof(SVR_Decoder));

    decoder->encoding = encoding;
    decoder->ready_frames = List_new();
    decoder->free_frames = List_new();
    decoder->write_offset = 0;
    decoder->frame_properties = SVR_FrameProperties_clone(frame_properties);
    SVR_LOCKABLE_INIT(decoder);

    if(encoding->openDecoder) {
        decoder->private_data = encoding->openDecoder(frame_properties);
    } else {
        decoder->private_data = NULL;
    }

    return decoder;
}

void SVR_Decoder_destroy(SVR_Decoder* decoder) {
    if(decoder->encoding->closeDecoder) {
        decoder->encoding->closeDecoder(decoder);
    }
}

int SVR_Decoder_decode(SVR_Decoder* decoder, void* data, size_t n) {
    decoder->encoding->decode(decoder, data, n);
    return SVR_Decoder_framesReady(decoder);
}

int SVR_Decoder_framesReady(SVR_Decoder* decoder) {
    return List_getSize(decoder->free_frames);
}

IplImage* SVR_Decoder_getFrame(SVR_Decoder* decoder) {
    IplImage* frame = NULL;

    SVR_LOCK(decoder);
    if(SVR_Decoder_framesReady(decoder)) {
        frame = List_remove(decoder->ready_frames, 0);
    }
    SVR_UNLOCK(decoder);

    return frame;
}

void SVR_Decoder_returnFrame(SVR_Decoder* decoder, IplImage* frame) {
    SVR_LOCK(decoder);
    List_append(decoder->free_frames, frame);
    SVR_UNLOCK(decoder);
}

/* Get the frame currently being built */
static IplImage* SVR_Decoder_getCurrentFrame(SVR_Decoder* decoder) {
    IplImage* frame;

    if(List_getSize(decoder->free_frames) == 0) {
        List_append(decoder->free_frames, SVR_FrameProperties_imageFromProperties(decoder->frame_properties));
    }

    frame = List_get(decoder->free_frames, 0);
    return frame;
}

static void SVR_Decoder_currentFrameComplete(SVR_Decoder* decoder) {
    if(List_getSize(decoder->free_frames) > 0) {
        /* Move current item from free frames to end of ready frames */
        List_append(decoder->ready_frames, List_remove(decoder->free_frames, 0));
        decoder->write_offset = 0;
    }
}

void SVR_Decoder_writePaddedFrameData(SVR_Decoder* decoder, void* data, size_t n) {
    IplImage* current_frame = SVR_Decoder_getCurrentFrame(decoder);
    size_t image_size = current_frame->imageSize;
    size_t chunk_size;
    int offset = 0;

    SVR_LOCK(decoder);
    while(offset < n) {
        current_frame = SVR_Decoder_getCurrentFrame(decoder);

        chunk_size = Util_min(image_size - decoder->write_offset, n - offset);
        memcpy(current_frame->imageData + decoder->write_offset, ((uint8_t*)data) + offset, chunk_size);
        decoder->write_offset = (decoder->write_offset + chunk_size) % image_size;

        if(decoder->write_offset == 0) {
            SVR_Decoder_currentFrameComplete(decoder);
        }
    }
    SVR_UNLOCK(decoder);
}

void SVR_Decoder_writeUnpaddedFrameData(SVR_Decoder* decoder, void* data, size_t n) {
    IplImage* current_frame = SVR_Decoder_getCurrentFrame(decoder);
    size_t image_size = current_frame->imageSize;
    size_t width_step = current_frame->widthStep;
    size_t row_width_remaining;
    size_t copy_size;
    int offset = 0;

    while(offset < n) {
        current_frame = SVR_Decoder_getCurrentFrame(decoder);

        row_width_remaining = current_frame->widthStep - (decoder->write_offset % width_step);
        copy_size = Util_min(row_width_remaining, n - offset);
        memcpy(current_frame->imageData + decoder->write_offset, ((uint8_t*)data) + offset, copy_size);

        decoder->write_offset += copy_size;
        offset += copy_size;

        /* Skip padding if we're at the end of a line */
        if(decoder->write_offset % width_step == current_frame->width) {
            decoder->write_offset = (decoder->write_offset + SVR_Decoder_getRowPadding(decoder)) % image_size;
        }

        /* Frame filled */
        if(decoder->write_offset == 0) {
            SVR_Decoder_currentFrameComplete(decoder);
        }
    }
}

int SVR_Decoder_getRowPadding(SVR_Decoder* decoder) {
    IplImage* frame;
    int padding;

    SVR_LOCK(decoder);
    frame = SVR_Decoder_getCurrentFrame(decoder);
    padding = frame->widthStep - frame->width;
    SVR_UNLOCK(decoder);

    return padding;
}
