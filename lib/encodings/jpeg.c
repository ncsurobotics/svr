
#include <svr.h>

#include <jpeglib.h>

#include "encoding_internal.h"

#define BUFFER_GROW_SIZE 1024
#define JPEG_DEFAULT_QUALITY 70

static void* openEncoder(SVR_FrameProperties* frame_properties, Dictionary* options);
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

#if JPEG_LIB_VERSION < 80

/* The following code is borrowed from libjpeg8 */

METHODDEF(void)
init_mem_source (j_decompress_ptr cinfo)
{
    /* no work necessary here */
}

METHODDEF(boolean)
fill_mem_input_buffer (j_decompress_ptr cinfo)
{
    static JOCTET mybuffer[4];

    /* Insert a fake EOI marker */
    mybuffer[0] = (JOCTET) 0xFF;
    mybuffer[1] = (JOCTET) JPEG_EOI;

    cinfo->src->next_input_byte = mybuffer;
    cinfo->src->bytes_in_buffer = 2;

    return TRUE;
}

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr * src = cinfo->src;

    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0) {
        while (num_bytes > (long) src->bytes_in_buffer) {
            num_bytes -= (long) src->bytes_in_buffer;
            (void) (*src->fill_input_buffer) (cinfo);
            /* note we assume that fill_input_buffer will never return FALSE,
             * so suspension need not be handled.
             */
        }
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
    /* no work necessary here */
}

METHODDEF(void)
jpeg_mem_src (j_decompress_ptr cinfo,
              unsigned char * inbuffer, unsigned long insize)
{
    struct jpeg_source_mgr * src;

    /* The source object is made permanent so that a series of JPEG images
     * can be read from the same buffer by calling jpeg_mem_src only before
     * the first one.
     */
    if (cinfo->src == NULL) {/* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                        sizeof(struct jpeg_source_mgr));
    }

    src = cinfo->src;
    src->init_source = init_mem_source;
    src->fill_input_buffer = fill_mem_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = (size_t) insize;
    src->next_input_byte = (JOCTET *) inbuffer;
}

#endif

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

static void* openEncoder(SVR_FrameProperties* frame_properties, Dictionary* options) {
    SVR_JpegEncoder* private_data = malloc(sizeof(SVR_JpegEncoder));
    int quality = JPEG_DEFAULT_QUALITY;;

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

    if(Dictionary_exists(options, "quality")) {
        quality = atoi(Dictionary_get(options, "quality"));
        if(quality < 5 || quality > 100) {
            SVR_log(SVR_WARNING, Util_format("Invalid JPEG quality %s. Falling back to default",
                                             Dictionary_get(options, "quality")));
            quality = JPEG_DEFAULT_QUALITY;
        }
    }
    
    jpeg_set_quality(&private_data->cinfo, quality, true);

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
