
#include <svr.h>

#include <jpeglib.h>

#include "encoding_internal.h"

#define BUFFER_GROW_SIZE 1024
#define JPEG_QUALITY 50

static void* openEncoder(SVR_FrameProperties* frame_properties);
static void closeEncoder(SVR_Encoder* encoder);
static void encode(SVR_Encoder* encoder, IplImage* frame);

static void* openDecoder(SVR_FrameProperties* frame_properties);
static void closeDecoder(SVR_Decoder* decoder);
static void decode(SVR_Decoder* decoder, void* data, size_t n);

SVR_Encoding SVR_ENCODING(jpeg) = {
        .name = "jpeg",
        .openEncoder = openEncoder,
        .closeEncoder = closeEncoder,
        .encode = encode,
        .openDecoder = openDecoder,
        .closeDecoder = closeDecoder,
        .decode = decode
};

typedef struct {
    struct jpeg_destination_mgr pub;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    SVR_Encoder* encoder;

    unsigned char* buffer;
    unsigned long buffer_size;
} SVR_JpegEncoder;

typedef struct {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row;

    unsigned char* buffer;
    unsigned long buffer_size;
    unsigned long bytes_needed;
    unsigned long bytes_received;
} SVR_JpegDecoder;

METHODDEF(void) init_svr_destination(j_compress_ptr cinfo) {
    SVR_JpegEncoder* private_data = (SVR_JpegEncoder*) cinfo->dest;

    private_data->pub.next_output_byte = private_data->buffer;
    private_data->pub.free_in_buffer = private_data->buffer_size;
}

METHODDEF(boolean) empty_svr_output_buffer(j_compress_ptr cinfo) {
    SVR_JpegEncoder* private_data = (SVR_JpegEncoder*) cinfo->dest;

    private_data->buffer = realloc(private_data->buffer, private_data->buffer_size + BUFFER_GROW_SIZE);
    private_data->pub.next_output_byte = private_data->buffer + private_data->buffer_size;
    private_data->pub.free_in_buffer = BUFFER_GROW_SIZE;
    private_data->buffer_size += BUFFER_GROW_SIZE;

    return true;
}

METHODDEF(void) term_svr_destination(j_compress_ptr cinfo) {
    SVR_JpegEncoder* private_data = (SVR_JpegEncoder*) cinfo->dest;
    uint32_t data_length = private_data->buffer_size - private_data->pub.free_in_buffer;
    uint32_t encoded_length = htonl(data_length);

    /* Write data */
    SVR_Encoder_provideData(private_data->encoder, &encoded_length, sizeof(encoded_length));
    SVR_Encoder_provideData(private_data->encoder, private_data->buffer, data_length);
}

static void* openEncoder(SVR_FrameProperties* frame_properties) {
    SVR_JpegEncoder* private_data = malloc(sizeof(SVR_JpegEncoder));

    private_data->cinfo.err = jpeg_std_error(&private_data->jerr);
    jpeg_create_compress(&private_data->cinfo);

    private_data->cinfo.image_width = frame_properties->width;
    private_data->cinfo.image_height = frame_properties->height;
    private_data->cinfo.input_components = frame_properties->channels;

    if(frame_properties->channels == 1) {
        private_data->cinfo.in_color_space = JCS_GRAYSCALE;
    } else {
        private_data->cinfo.in_color_space = JCS_RGB;
    }

    jpeg_set_defaults(&private_data->cinfo);
    jpeg_set_quality(&private_data->cinfo, JPEG_QUALITY, true);

    private_data->cinfo.dest = (struct jpeg_destination_mgr*) private_data;
    private_data->buffer_size = BUFFER_GROW_SIZE;
    private_data->buffer = malloc(private_data->buffer_size);
    private_data->pub.next_output_byte = private_data->buffer;
    private_data->pub.free_in_buffer = private_data->buffer_size;
    private_data->pub.init_destination = init_svr_destination;
    private_data->pub.empty_output_buffer = empty_svr_output_buffer;
    private_data->pub.term_destination = term_svr_destination;

    return private_data;
}

static void closeEncoder(SVR_Encoder* encoder) {
    SVR_JpegEncoder* private_data = encoder->private_data;
    jpeg_destroy_compress(&private_data->cinfo);
    free(private_data->buffer);
    free(private_data);
}

static void encode(SVR_Encoder* encoder, IplImage* frame) {
    SVR_JpegEncoder* private_data = encoder->private_data;
    JSAMPROW row;

    private_data->encoder = encoder;
    jpeg_start_compress(&private_data->cinfo, true);

    for(int r = 0; r < frame->height; r++) {
        row = (JSAMPROW) frame->imageData + (r * frame->widthStep);
        jpeg_write_scanlines(&private_data->cinfo, (JSAMPARRAY) &row, 1);
    }

    jpeg_finish_compress(&private_data->cinfo);
}

static void* openDecoder(SVR_FrameProperties* frame_properties) {
    SVR_JpegDecoder* private_data = malloc(sizeof(SVR_JpegDecoder));

    private_data->cinfo.err = jpeg_std_error(&private_data->jerr);
    jpeg_create_decompress(&private_data->cinfo);

    private_data->buffer = NULL;
    private_data->buffer_size = 0;
    private_data->bytes_needed = 0;
    private_data->bytes_received = 0;

    private_data->row = malloc(frame_properties->width * frame_properties->channels);

    return private_data;
}

static void closeDecoder(SVR_Decoder* decoder) {
    SVR_JpegDecoder* private_data = decoder->private_data;
    jpeg_destroy_decompress(&private_data->cinfo);
    free(private_data->buffer);
    free(private_data->row);
    free(private_data);
}

static void decode(SVR_Decoder* decoder, void* data, size_t n) {
    SVR_JpegDecoder* private_data = decoder->private_data;
    unsigned long chunk_size;

    while(n) {
        if(private_data->bytes_needed == 0) {
            private_data->bytes_received = 0;

            /* Read the first 4 bytes which give us the size of the encoded
               frame */
            private_data->bytes_needed = ntohl(((uint32_t*)data)[0]);
            data = ((uint8_t*)data) + sizeof(uint32_t);
            n -= sizeof(uint32_t);

            if(private_data->buffer_size < private_data->bytes_needed) {
                private_data->buffer = realloc(private_data->buffer, private_data->bytes_needed);
                private_data->buffer_size = private_data->bytes_needed;
            }
        } else {
            chunk_size = Util_min(n, private_data->bytes_needed);
            memcpy(private_data->buffer + private_data->bytes_received, data, chunk_size);
            private_data->bytes_received += chunk_size;

            n -= chunk_size;
            data = ((uint8_t*)data) + chunk_size;

            if(private_data->bytes_received == private_data->bytes_needed) {
                jpeg_mem_src(&private_data->cinfo, private_data->buffer, private_data->bytes_needed);
                jpeg_read_header(&private_data->cinfo, true);
                jpeg_start_decompress(&private_data->cinfo);

                for(int r = 0; r < decoder->frame_properties->height; r++) {
                    jpeg_read_scanlines(&private_data->cinfo, &private_data->row, 1);
                    SVR_Decoder_writeUnpaddedFrameData(decoder, private_data->row,
                            decoder->frame_properties->width * decoder->frame_properties->channels);
                }
                jpeg_finish_decompress(&private_data->cinfo);

                private_data->bytes_needed = 0;
            }
        }
    }
}
