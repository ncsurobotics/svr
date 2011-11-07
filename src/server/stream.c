
#include <svr.h>
#include <svr/server/svr.h>

static void SVRs_Stream_initializeEncoder(SVRs_Stream* stream);
static void SVRs_Stream_reallocateTemporaryFrames(SVRs_Stream* stream);
static IplImage* SVRs_Stream_preprocessFrame(SVRs_Stream* stream, IplImage* frame);

SVRs_Stream* SVRs_Stream_new(const char* name) {
    SVRs_Stream* stream = malloc(sizeof(SVRs_Stream));

    stream->client = NULL;
    stream->name = strdup(name);
    stream->state = SVR_PAUSED;

    stream->source = NULL;
    stream->frame_properties = SVR_FrameProperties_new();
    stream->encoder = NULL;
    stream->encoding = SVR_Encoding_getByName("raw");

    stream->payload_buffer_size = 8 * 1024;
    stream->payload_buffer = malloc(stream->payload_buffer_size);

    SVR_LOCKABLE_INIT(stream);

    return stream;
}

void SVRs_Stream_setClient(SVRs_Stream* stream, SVRs_Client* client) {
    SVR_LOCK(stream);
    if(stream->client) {
        SVR_UNREF(stream->client);
        stream->client = NULL;
    }

    stream->client = client;
    SVR_REF(stream->client);
    SVR_UNLOCK(stream);
}

static void SVRs_Stream_initializeEncoder(SVRs_Stream* stream) {
    SVR_LOCK(stream);
    if(stream->encoder) {
        SVR_Encoder_destroy(stream->encoder);
    }

    stream->encoder = SVR_Encoder_new(stream->encoding, stream->frame_properties);
    SVR_UNLOCK(stream);
}


int SVRs_Stream_attachSource(SVRs_Stream* stream, SVRs_Source* source) {
    if(stream->state == SVR_UNPAUSED) {
        return SVR_INVALIDSTATE;
    }

    stream->source = source;
    if(stream->frame_properties) {
        SVR_FrameProperties_destroy(stream->frame_properties);
    }

    stream->frame_properties = SVR_FrameProperties_clone(SVRs_Source_getFrameProperties(source));
    SVRs_Source_registerStream(source, stream);

    return SVR_SUCCESS;
}

int SVRs_Stream_detachSource(SVRs_Stream* stream) {
    if(stream->state == SVR_UNPAUSED) {
        return SVR_INVALIDSTATE;
    }

    if(stream->source) {
        SVRs_Source_unregisterStream(stream->source, stream);
    }

    stream->source = NULL;
    stream->frame_properties = NULL;
    return SVR_SUCCESS;
}

int SVRs_Stream_setEncoding(SVRs_Stream* stream, SVR_Encoding* encoding) {
    if(stream->state == SVR_UNPAUSED) {
        return SVR_INVALIDSTATE;
    }

    stream->encoding = encoding;
    return SVR_SUCCESS;
}

static void SVRs_Stream_reallocateTemporaryFrames(SVRs_Stream* stream) {
    SVR_FrameProperties* source_frame_properties = SVRs_Source_getFrameProperties(stream->source);
    SVR_FrameProperties* temp_frame_properties;

    bool resize = (stream->frame_properties->width != source_frame_properties->width ||
                   stream->frame_properties->height != source_frame_properties->height);
    bool color_convert = (stream->frame_properties->channels != source_frame_properties->channels);

    if(stream->temp_frame[0]) {
        cvReleaseImage(&stream->temp_frame[0]);
    }

    if(stream->temp_frame[1]) {
        cvReleaseImage(&stream->temp_frame[1]);
    }

    stream->temp_frame[0] = NULL;
    stream->temp_frame[1] = NULL;

    if(resize && color_convert) {
        temp_frame_properties = SVR_FrameProperties_clone(stream->frame_properties);
        temp_frame_properties->channels = source_frame_properties->channels;

        stream->temp_frame[0] = SVR_FrameProperties_imageFromProperties(temp_frame_properties);
        stream->temp_frame[1] = SVR_FrameProperties_imageFromProperties(stream->frame_properties);

        SVR_FrameProperties_destroy(temp_frame_properties);
    } else if(resize || color_convert) {
        stream->temp_frame[0] = SVR_FrameProperties_imageFromProperties(stream->frame_properties);
    }
}

int SVRs_Stream_resize(SVRs_Stream* stream, int width, int height) {
    if(stream->state == SVR_UNPAUSED || stream->source == NULL) {
        return SVR_INVALIDSTATE;
    }

    stream->frame_properties->width = width;
    stream->frame_properties->height = height;

    SVRs_Stream_reallocateTemporaryFrames(stream);

    return SVR_SUCCESS;
}

int SVRs_Stream_setChannels(SVRs_Stream* stream, int channels) {
    if(channels != 1 && channels != 3) {
        return SVR_INVALIDARGUMENT;
    }

    if(stream->state == SVR_UNPAUSED || stream->source == NULL) {
        return SVR_INVALIDSTATE;
    }

    stream->frame_properties->channels = channels;

    SVRs_Stream_reallocateTemporaryFrames(stream);

    return SVR_SUCCESS;
}

/**
 * \brief Pause a stream
 *
 * Stop the delivery of frame data until the stream is started again
 *
 * \param stream The stream to pause
 */
void SVRs_Stream_pause(SVRs_Stream* stream) {
    SVR_LOCK(stream);
    stream->state = SVR_PAUSED;
    SVR_UNLOCK(stream);
}

void SVRs_Stream_unpause(SVRs_Stream* stream) {
    SVR_LOCK(stream);
    stream->state = SVR_UNPAUSED;
    SVRs_Stream_initializeEncoder(stream);
    SVR_UNLOCK(stream);
}

void SVRs_Stream_destroy(SVRs_Stream* stream) {
    /* Detach the source without a lock to avoid a deadlock */
    SVRs_Stream_detachSource(stream);

    SVR_LOCK(stream);

    if(stream->name) {
        free(stream->name);
        stream->name = NULL;
    }

    if(stream->payload_buffer) {
        free(stream->payload_buffer);
        stream->payload_buffer = NULL;
    }

    if(stream->encoder) {
        SVR_Encoder_destroy(stream->encoder);
        stream->encoder = NULL;
    }

    if(stream->temp_frame[0]) {
        cvReleaseImage(&stream->temp_frame[0]);
    }

    if(stream->temp_frame[1]) {
        cvReleaseImage(&stream->temp_frame[1]);
    }

    SVR_UNREF(stream->client);
    SVR_UNLOCK(stream);
    free(stream);
}

static IplImage* SVRs_Stream_preprocessFrame(SVRs_Stream* stream, IplImage* frame) {
    SVR_FrameProperties* source_frame_properties = SVRs_Source_getFrameProperties(stream->source);
    bool resize = (stream->frame_properties->width != source_frame_properties->width ||
                   stream->frame_properties->height != source_frame_properties->height);
    bool color_convert = (stream->frame_properties->channels != source_frame_properties->channels);

    if(resize && color_convert) {
        cvResize(frame, stream->temp_frame[0], CV_INTER_NN);

        if(stream->frame_properties->channels == 1) {
            cvCvtColor(stream->temp_frame[0], stream->temp_frame[1], CV_RGB2GRAY);
        } else {
            cvCvtColor(stream->temp_frame[0], stream->temp_frame[1], CV_GRAY2RGB);
        }

        return stream->temp_frame[1];

    } else if(resize) {
        cvResize(frame, stream->temp_frame[0], CV_INTER_NN);
        return stream->temp_frame[0];

    } else if(color_convert) {
        if(stream->frame_properties->channels == 1) {
            cvCvtColor(frame, stream->temp_frame[0], CV_RGB2GRAY);
        } else {
            cvCvtColor(frame, stream->temp_frame[0], CV_GRAY2RGB);
        }

        return stream->temp_frame[0];
    }

    return frame;
}

void SVRs_Stream_inputSourceFrame(SVRs_Stream* stream, IplImage* frame) {
    SVR_Message* message;

    SVR_LOCK(stream);
    /* Ignore data if paused */
    if(stream->state == SVR_PAUSED || stream->client == NULL) {
        SVR_UNLOCK(stream);
        return;
    }

    /* Build the data message */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Data");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->name);
    message->payload = stream->payload_buffer;

    frame = SVRs_Stream_preprocessFrame(stream, frame);
    SVR_Encoder_encode(stream->encoder, frame);

    /* Send all the encoded data out in chunks */
    while(SVR_Encoder_dataReady(stream->encoder) > 0) {
        /* Get part of payload */
        message->payload_size = SVR_Encoder_readData(stream->encoder,
                                                     message->payload,
                                                     stream->payload_buffer_size);

        /* Send message */
        if(SVRs_Client_sendMessage(stream->client, message) < 0) {
            SVRs_Stream_pause(stream);
            SVR_log(SVR_DEBUG, "Can not send message");
            break;
        }
    }

    SVR_Message_release(message);
    SVR_UNLOCK(stream);
}
