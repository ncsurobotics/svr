
#include <svr.h>
#include <svrd.h>

static void SVRD_Stream_initializeEncoder(SVRD_Stream* stream);
static void SVRD_Stream_reallocateTemporaryFrames(SVRD_Stream* stream);
static IplImage* SVRD_Stream_preprocessFrame(SVRD_Stream* stream, IplImage* frame);
static void* SVRD_Stream_worker(void* _stream);

SVRD_Stream* SVRD_Stream_new(const char* name) {
    SVRD_Stream* stream = malloc(sizeof(SVRD_Stream));

    stream->client = NULL;
    stream->name = strdup(name);
    stream->state = SVR_PAUSED;

    stream->source = NULL;
    stream->frame_properties = SVR_FrameProperties_new();
    stream->encoder = NULL;
    stream->encoding = NULL;
    stream->encoding_options = NULL;

    stream->payload_buffer_size = 8 * 1024;
    stream->payload_buffer = malloc(stream->payload_buffer_size);

    stream->priority = 1;

    stream->temp_frame[0] = NULL;
    stream->temp_frame[1] = NULL;

    stream->drop_counter = 0;
    stream->drop_rate = 0;

    SVR_LOCKABLE_INIT(stream);

    /* Set default encoding */
    SVRD_Stream_setEncoding(stream, "raw");

    return stream;
}

void SVRD_Stream_setClient(SVRD_Stream* stream, SVRD_Client* client) {
    SVR_LOCK(stream);
    if(stream->client) {
        SVR_UNREF(stream->client);
        stream->client = NULL;
    }

    stream->client = client;
    SVR_REF(stream->client);
    SVR_UNLOCK(stream);
}

static void SVRD_Stream_initializeEncoder(SVRD_Stream* stream) {
    SVR_LOCK(stream);
    if(stream->encoder) {
        SVR_Encoder_destroy(stream->encoder);
    }

    stream->encoder = SVR_Encoder_new(stream->encoding, stream->encoding_options, stream->frame_properties);
    SVR_UNLOCK(stream);
}


int SVRD_Stream_attachSource(SVRD_Stream* stream, SVRD_Source* source) {
    if(stream->state == SVR_UNPAUSED || SVRD_Source_getFrameProperties(source) == NULL) {
        return SVR_INVALIDSTATE;
    }

    if(stream->source) {
        SVR_UNREF(stream->source);
    }

    stream->source = source;
    if(stream->frame_properties) {
        SVR_FrameProperties_destroy(stream->frame_properties);
    }

    stream->frame_properties = SVR_FrameProperties_clone(SVRD_Source_getFrameProperties(source));

    return SVR_SUCCESS;
}

int SVRD_Stream_detachSource(SVRD_Stream* stream) {
    if(stream->state == SVR_UNPAUSED) {
        return SVR_INVALIDSTATE;
    }

    stream->source = NULL;
    stream->frame_properties = NULL;
    return SVR_SUCCESS;
}

/* Source is closing */
void SVRD_Stream_sourceClosing(SVRD_Stream* stream) {
    SVR_Message* message;

    /* Pause stream */
    SVRD_Stream_pause(stream);

    /* Detach source */
    SVRD_Stream_detachSource(stream);

    /* Notify client that source has orphaned the stream */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.orphaned");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->name);

    SVRD_Client_sendMessage(stream->client, message);
    SVR_Message_release(message);
}

int SVRD_Stream_setEncoding(SVRD_Stream* stream, const char* encoding_descriptor) {
    Dictionary* options;
    SVR_Encoding* encoding;

    if(stream->state == SVR_UNPAUSED) {
        return SVR_INVALIDSTATE;
    }

    options = SVR_parseOptionString(encoding_descriptor);
    if(options == NULL) {
        return SVR_PARSEERROR;
    }

    encoding = SVR_Encoding_getByName(Dictionary_get(options, "%name"));
    if(encoding == NULL) {
        SVR_freeParsedOptionString(options);
        return SVR_NOSUCHENCODING;
    }

    if(stream->encoding_options) {
        SVR_freeParsedOptionString(stream->encoding_options);
    }
    stream->encoding = encoding;
    stream->encoding_options = options;

    return SVR_SUCCESS;
}

/* Only process 1 of every rate frames */
int SVRD_Stream_setDropRate(SVRD_Stream* stream, int rate) {
    stream->drop_rate = rate;
    stream->drop_counter = 0;

    return SVR_SUCCESS;
}

int SVRD_Stream_setPriority(SVRD_Stream* stream, short priority) {
    stream->priority = priority;
    return SVR_SUCCESS;
}

static void SVRD_Stream_reallocateTemporaryFrames(SVRD_Stream* stream) {
    SVR_FrameProperties* source_frame_properties = SVRD_Source_getFrameProperties(stream->source);
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

int SVRD_Stream_resize(SVRD_Stream* stream, int width, int height) {
    if(stream->state == SVR_UNPAUSED || stream->source == NULL) {
        return SVR_INVALIDSTATE;
    }

    stream->frame_properties->width = width;
    stream->frame_properties->height = height;

    SVRD_Stream_reallocateTemporaryFrames(stream);

    return SVR_SUCCESS;
}

int SVRD_Stream_setChannels(SVRD_Stream* stream, int channels) {
    if(channels != 1 && channels != 3) {
        return SVR_INVALIDARGUMENT;
    }

    if(stream->state == SVR_UNPAUSED || stream->source == NULL) {
        return SVR_INVALIDSTATE;
    }

    stream->frame_properties->channels = channels;

    SVRD_Stream_reallocateTemporaryFrames(stream);

    return SVR_SUCCESS;
}

/**
 * \brief Pause a stream
 *
 * Stop the delivery of frame data until the stream is started again
 *
 * \param stream The stream to pause
 */
void SVRD_Stream_pause(SVRD_Stream* stream) {
    SVR_LOCK(stream);
    if(stream->state == SVR_PAUSED) {
        SVR_UNLOCK(stream);
        return;
    }
    stream->state = SVR_PAUSED;
    SVR_UNLOCK(stream);

    /* Request that the source dismiss the streams getFrame request */
    SVRD_Source_dismissPausedStreams(stream->source);
}

void SVRD_Stream_unpause(SVRD_Stream* stream) {
    SVR_LOCK(stream);
    if(stream->state == SVR_PAUSED && stream->client != NULL &&
       stream->encoding != NULL && stream->source != NULL) {
        stream->state = SVR_UNPAUSED;
        SVRD_Stream_initializeEncoder(stream);
        pthread_create(&stream->worker, NULL, SVRD_Stream_worker, stream);
    }
    SVR_UNLOCK(stream);
}

void SVRD_Stream_destroy(SVRD_Stream* stream) {
    SVR_LOCK(stream);

    SVRD_Stream_pause(stream);
    pthread_join(stream->worker, NULL);

    /* Detach the source without a lock to avoid a deadlock */
    SVRD_Stream_detachSource(stream);

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

static IplImage* SVRD_Stream_preprocessFrame(SVRD_Stream* stream, IplImage* frame) {
    SVR_FrameProperties* source_frame_properties = SVRD_Source_getFrameProperties(stream->source);
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

static void* SVRD_Stream_worker(void* _stream) {
    SVRD_Stream* stream = (SVRD_Stream*) _stream;
    SVRD_SourceFrame* source_frame = NULL;
    IplImage* frame;
    SVR_Message* message;

    while(stream->state == SVR_UNPAUSED) {
        source_frame = SVRD_Source_getFrame(stream->source, stream, source_frame);

        if(source_frame == NULL) {
            if(stream->state == SVR_UNPAUSED) {
                /* Source closing */
                SVRD_Stream_sourceClosing(stream);
            }

            break;
        }

        if(stream->drop_rate) {
            stream->drop_counter = (stream->drop_counter + 1) % stream->drop_rate;

            if(stream->drop_counter != 0) {
                continue;
            }
        }


        frame = SVRD_Stream_preprocessFrame(stream, source_frame->frame);
        SVR_Encoder_encode(stream->encoder, frame);

        /* Send all the encoded data out in chunks */
        while(SVR_Encoder_dataReady(stream->encoder) > 0) {
            /* Build the data message */
            message = SVR_Message_new(2);
            message->components[0] = "Data";
            message->components[1] = stream->name;
            message->payload = stream->payload_buffer;

            /* Get part of payload */
            message->payload_size = SVR_Encoder_readData(stream->encoder,
                                                         message->payload,
                                                         stream->payload_buffer_size);

            /* Send message */
            if(SVRD_Client_sendMessage(stream->client, message) < 0) {
                SVRD_Stream_pause(stream);
                SVR_log(SVR_DEBUG, "Can not send message");
                break;
            }

            SVR_Message_release(message);
        }
    }

    if(source_frame) {
        SVR_UNREF(source_frame);
    }

    return NULL;
}
