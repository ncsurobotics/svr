/**
 * \file
 * \brief Encoding
 */

#include <svr.h>
#include <seawolf.h>

#include "encodings/encoding_internal.h"
#include "encodings/encodings.h"

static void SVR_Encoding_registerDefaultEncodings(void);

static Dictionary* encodings = NULL;

/**
 * \defgroup Encoding Encoding
 * \ingroup Misc
 * \brief Stream encoding and decoding
 *
 * An encoding (SVR_Encoding) is an implementations of both an encoder and
 * decoder for a particular image/video encoding method. Once an encoding is
 * registered with the SVR library, it can be used with streams and sources. An
 * encoding can be retreive by name by calling SVR_Encoding_getByName. The
 * encoding instance can then be used to initialize multiple encoders and
 * decoders with calls to SVR_Encoder_new and SVR_Decoder_new respectively.
 *
 * An encoder processes frames and produces raw, encoded data. Conversely, a
 * decoder processes raw, encoded data and produces decoded frames. The APIs
 * below, provide this functionality.
 *
 * An encoder maintains an internal, circular buffer of encoded data. As the
 * data is read out, internal buffer space is freed. The buffer will grow as
 * large as necessary to keep all unread data buffered.
 *
 * A decoder maintains internal free, and ready frames lists. The free frames
 * list is the list of frames which are available for the decoder to buffer
 * incoming frames. The ready frames list is the list of fully buffered frames
 * available to be returned by a call to SVR_Decoder_getFrame. Decoded frames
 * are placed into the free frames list with a call to SVR_Decoder_returnFrame,
 * so it is important that frames obtained by a call to SVR_Decoder_getFrame be
 * returned to avoid memory leaks and excessive memory reallocation.
 *
 * \{
 */

/**
 * \brief Initialize encoding module
 *
 * Initialize encoding module
 */
void SVR_Encoding_init(void) {
    encodings = Dictionary_new();
    SVR_Encoding_registerDefaultEncodings();
}

/**
 * \brief Register the built-in encodings
 *
 * Register the built-in encodings
 */
static void SVR_Encoding_registerDefaultEncodings(void) {
    SVR_Encoding_register(&SVR_ENCODING(raw));
    SVR_Encoding_register(&SVR_ENCODING(jpeg));
}

/**
 * \brief Close the encoding module
 *
 * Deallocate global data structures used by the encoding module
 */
void SVR_Encoding_close(void) {
    Dictionary_destroy(encodings);
}

/**
 * \brief Retrieve an encoding by name
 *
 * Retrieve an encoding by name
 *
 * \param name Name of the encoding
 * \return The encoding object, or NULL if no encoding exists with the given name
 */
SVR_Encoding* SVR_Encoding_getByName(const char* name) {
    return Dictionary_get(encodings, name);
}

/**
 * \brief Register an encoding
 *
 * Register an encoding
 *
 * \param encoding The encoding to register. The encoding will be associated
 * with the name given by encoding->name
 * \return 0 on success, -1 on failure (encoding already exists)
 */
int SVR_Encoding_register(SVR_Encoding* encoding) {
    if(Dictionary_exists(encodings, encoding->name)) {
        return -1;
    }

    Dictionary_set(encodings, encoding->name, encoding);

    return 0;
}

/**
 * \brief Open a new encoder
 *
 * Open a new instance of an encoder with the given encoding, encoder options,
 * and frame properties
 *
 * \param encoding The encoding to use
 * \param encoding_options A dictionary giving any options/arguments to the encoder
 * \param frame_properties The frame properties for frames to be encoded
 * \return A new encoder
 */
SVR_Encoder* SVR_Encoder_new(SVR_Encoding* encoding, Dictionary* encoding_options, SVR_FrameProperties* frame_properties) {
    SVR_Encoder* encoder = malloc(sizeof(SVR_Encoder));

    encoder->encoding = encoding;
    encoder->buffer = NULL;
    encoder->write_index = 0;
    encoder->read_index = 0;
    encoder->buffer_size = 1;
    SVR_LOCKABLE_INIT(encoder);

    if(encoding->openEncoder) {
        encoder->private_data = encoding->openEncoder(frame_properties, encoding_options);
    } else {
        encoder->private_data = NULL;
    }

    return encoder;
}

/**
 * \brief Destroy an encoder
 *
 * Destroy and deallocate the given encoder
 *
 * \param encoder The encoder to destroy
 */
void SVR_Encoder_destroy(SVR_Encoder* encoder) {
    if(encoder->encoding->closeEncoder) {
        encoder->encoding->closeEncoder(encoder);
    }

    if(encoder->buffer) {
        free(encoder->buffer);
    }

    free(encoder);
}

/**
 * \brief Encode a frame
 *
 * Encode a frame
 *
 * \param encoder The encoder to use to process the frame
 * \param frame The frame to encode
 * \return The number of encoded bytes available to be read
 */
size_t SVR_Encoder_encode(SVR_Encoder* encoder, IplImage* frame) {
    encoder->encoding->encode(encoder, frame);
    return SVR_Encoder_dataReady(encoder);
}

/**
 * \brief Get the number of bytes ready to be read
 *
 * Get the amount of encoded data ready to be read expressed in bytes
 *
 * \param encoder An encoder instance
 * \return The number of bytes ready to be read
 */
size_t SVR_Encoder_dataReady(SVR_Encoder* encoder) {
    if(encoder->read_index <= encoder->write_index) {
        return encoder->write_index - encoder->read_index;
    } else {
        return (encoder->buffer_size - encoder->read_index) + encoder->write_index;
    }
}

/**
 * \brief Get encoded data
 *
 * Read up to buffer_size bytes from the encoder output buffer into the provided
 * buffer. This call will automatically advance the interal read index.
 *
 * \param encoder An encoder instance to read from
 * \param buffer Destination address to write to
 * \param buffer_size Size of the destination buffer. The lesser of this and the
 * number of bytes ready will be stored to buffer.
 * \return Number of bytes stored to the buffer
 */
size_t SVR_Encoder_readData(SVR_Encoder* encoder, void* buffer, size_t buffer_size) {
    size_t read_size;
    size_t first_chunk;

    SVR_LOCK(encoder);
    read_size = Util_min(SVR_Encoder_dataReady(encoder), buffer_size);
    first_chunk = Util_min(read_size, encoder->buffer_size - encoder->read_index);

    memcpy(buffer, ((uint8_t*)encoder->buffer) + encoder->read_index, first_chunk);
    memcpy(((uint8_t*)buffer) + first_chunk, ((uint8_t*)encoder->buffer), read_size - first_chunk);
    encoder->read_index = (encoder->read_index + read_size) % encoder->buffer_size;
    SVR_UNLOCK(encoder);

    return read_size;
}

/**
 * \private
 * \brief Store encoded data
 *
 * This call shall be used by encoding implementations to provide encoded data
 * for buffering before returning to the user.
 *
 * \param encoder The associated encoder instance
 * \param data Pointer to the encoded data
 * \param n Number of bytes to store
 */
void SVR_Encoder_provideData(SVR_Encoder* encoder, void* data, size_t n) {
    size_t used_space;
    size_t free_space;
    size_t end_space;
    void* temp_space;

    SVR_LOCK(encoder);
    used_space = SVR_Encoder_dataReady(encoder);
    free_space = encoder->buffer_size - used_space;

    if(free_space <= n) {
        if(encoder->write_index < encoder->read_index) {
            /* Temporarily store unread data */
            temp_space = malloc(used_space);
            SVR_Encoder_readData(encoder, temp_space, used_space);

            /* Grow buffer and restore data */
            encoder->buffer = realloc(encoder->buffer, used_space + n + 1);
            memcpy(encoder->buffer, temp_space, used_space);
            encoder->read_index = 0;
            free(temp_space);

            encoder->write_index = used_space;
        } else{
            encoder->buffer = realloc(encoder->buffer, used_space + n + 1);
        }

        encoder->buffer_size = used_space + n + 1;
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

/**
 * \brief Open a decoder
 *
 * Open a new decoder using the given encoding and frame properties
 *
 * \param encoding Encoding type to use
 * \param frame_properties Properties of the frames which will be decoded
 * \return A new decoder
 */
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

/**
 * \brief Destroy a decoder
 *
 * Destroy the given decoder
 *
 * \param decoder A decoder instance
 */
void SVR_Decoder_destroy(SVR_Decoder* decoder) {
    IplImage* frame;

    if(decoder->encoding->closeDecoder) {
        decoder->encoding->closeDecoder(decoder);
    }

    SVR_FrameProperties_destroy(decoder->frame_properties);

    /* Free frames in free frames list */
    for(int i = 0; (frame = List_get(decoder->free_frames, i)) != NULL; i++) {
        cvReleaseImage(&frame);
    }
    List_destroy(decoder->free_frames);

    /* Free frames in ready frames list */
    for(int i = 0; (frame = List_get(decoder->ready_frames, i)) != NULL; i++) {
        cvReleaseImage(&frame);
    }
    List_destroy(decoder->ready_frames);

    free(decoder);
}

/**
 * \brief Provide data to be decoded
 *
 * Provide new encoded data to the decoder
 *
 * \param decoder A decoder instance
 * \param data Pointer to the encoded data
 * \param n Number of bytes to be read from the data buffer
 * \return Number of new decoded frames available
 */
int SVR_Decoder_decode(SVR_Decoder* decoder, void* data, size_t n) {
    decoder->encoding->decode(decoder, data, n);
    return SVR_Decoder_framesReady(decoder);
}

/**
 * \brief Get number of frames ready
 *
 * Get number of frames ready to be retrieved
 *
 * \param decoder A decoder instance
 * \return Number of frames ready
 */
int SVR_Decoder_framesReady(SVR_Decoder* decoder) {
    return List_getSize(decoder->ready_frames);
}

/**
 * \brief Get a decoded frame
 *
 * Get the next decoded frame. If one is not available, NULL shall be
 * returned. Once the caller no longer needs the frames they should be returned
 * to the decoder with a call to SVR_Decode_returnFrame.
 *
 * \param decoder A decoder instance
 * \return The next frame ready, or NULL if no frames available
 */
IplImage* SVR_Decoder_getFrame(SVR_Decoder* decoder) {
    IplImage* frame = NULL;

    SVR_LOCK(decoder);
    if(SVR_Decoder_framesReady(decoder)) {
        frame = List_remove(decoder->ready_frames, 0);
    }
    SVR_UNLOCK(decoder);

    return frame;
}

/**
 * \brief Return a frame to the decoder
 *
 * Return a frame obtained by a call to SVR_Decoder_getFrame to the decoder. The
 * frame will be reused for future frames.
 *
 * \param decoder A decoder instance
 * \param frame A frame obtained by a call to SVR_Decoder_getFrame
 */
void SVR_Decoder_returnFrame(SVR_Decoder* decoder, IplImage* frame) {
    SVR_LOCK(decoder);
    List_append(decoder->free_frames, frame);
    SVR_UNLOCK(decoder);
}

/**
 * \brief Get the current, buffering frame
 *
 * Get the frame currently being buffered, or allocated and return a new frame
 * if there are already no new frames available
 *
 * \param decoder A decoder instance
 */
static IplImage* SVR_Decoder_getCurrentFrame(SVR_Decoder* decoder) {
    IplImage* frame;

    /* Allocate a new frame */
    if(List_getSize(decoder->free_frames) == 0) {
        frame = SVR_FrameProperties_imageFromProperties(decoder->frame_properties);
        List_append(decoder->free_frames, frame);
    }

    frame = List_get(decoder->free_frames, 0);
    return frame;
}

/**
 * \brief Mark the currently buffering frames as complete
 *
 * Called once the currently buffering frame has been completely buffered. This
 * call makes the currently buffering frame available to the client through
 * SVR_Decoder_getFrame.
 *
 * \param decoder A decoder instance
 */
static void SVR_Decoder_currentFrameComplete(SVR_Decoder* decoder) {
    if(List_getSize(decoder->free_frames) > 0) {
        /* Move current item from free frames to end of ready frames */
        List_append(decoder->ready_frames, List_remove(decoder->free_frames, 0));
        decoder->write_offset = 0;
    }
}

/**
 * \private
 * \brief Provide decoded frame data with padding
 *
 * Provide decoded frame data which is already correctly row padded. This
 * function is only to be called by encoding implementations.
 *
 * \param decoder A decoder instance
 * \param data Decoded frame data
 * \param n Number of decoded bytes
 */
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
        offset += chunk_size;

        if(decoder->write_offset == 0) {
            SVR_Decoder_currentFrameComplete(decoder);
        }
    }
    SVR_UNLOCK(decoder);
}

/**
 * \private
 * \brief Provide decoded frame data without padding
 *
 * Provide decoded frame data without row padding. This function should only be
 * called by encoding implementations.
 *
 * \param decoder A decoder instance
 * \param data Decoded frame data
 * \param n Number of decoded bytes
 */
void SVR_Decoder_writeUnpaddedFrameData(SVR_Decoder* decoder, void* data, size_t n) {
    IplImage* current_frame;
    size_t image_size;
    size_t width_step;
    size_t row_width_remaining;
    size_t copy_size;
    int offset = 0;

    /* Not only is this faster, but it's necessary since the "end-of-row"
       calculations will fail for 0 padding */
    if(SVR_Decoder_getRowPadding(decoder) == 0) {
        SVR_Decoder_writePaddedFrameData(decoder, data, n);
        return;
    }

    current_frame = SVR_Decoder_getCurrentFrame(decoder);
    image_size = current_frame->imageSize;
    width_step = current_frame->widthStep;

    while(offset < n) {
        current_frame = SVR_Decoder_getCurrentFrame(decoder);

        row_width_remaining = (current_frame->width * current_frame->nChannels) - (decoder->write_offset % width_step);
        copy_size = Util_min(row_width_remaining, n - offset);
        memcpy(current_frame->imageData + decoder->write_offset, ((uint8_t*)data) + offset, copy_size);

        decoder->write_offset += copy_size;
        offset += copy_size;

        /* Skip padding if we're at the end of a line */
        if(decoder->write_offset % width_step == (current_frame->width * current_frame->nChannels)) {
            decoder->write_offset = (decoder->write_offset + SVR_Decoder_getRowPadding(decoder)) % image_size;
        }

        /* Frame filled */
        if(decoder->write_offset == 0) {
            SVR_Decoder_currentFrameComplete(decoder);
        }
    }
}

/**
 * \private
 * \brief Get the row padding
 *
 * Get the amount of row padding, i.e. the number of additional bytes at the end
 * of each row used to align rows at more optimal memory boundaries. This call
 * is provide for the benefit of encoding implementations
 *
 * \param decoder A decoder instance
 * \return The number of pad bytes in the decoded frames
 */ 
int SVR_Decoder_getRowPadding(SVR_Decoder* decoder) {
    IplImage* frame;
    int padding;

    SVR_LOCK(decoder);
    frame = SVR_Decoder_getCurrentFrame(decoder);
    padding = frame->widthStep - (frame->width * frame->nChannels);
    SVR_UNLOCK(decoder);

    return padding;
}

/** \} */
