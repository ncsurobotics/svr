
#include <svr.h>
#include <svr/server/svr.h>

SVRs_Stream* SVRs_Stream_new(const char* name) {
    SVRs_Stream* stream = malloc(sizeof(SVRs_Stream));

    stream->client = NULL;
    stream->name = strdup(name);
    stream->state = SVR_PAUSED;

    stream->source = NULL;
    stream->frame_properties = NULL;
    stream->encoder = NULL;
    stream->encoding = SVR_Encoding_getByName("raw");

    stream->frame_filters = List_new();

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

void SVRs_Stream_attachSource(SVRs_Stream* stream, SVRs_Source* source) {
    stream->source = source;
    stream->frame_properties = SVRs_Source_getFrameProperties(source);
    SVRs_Source_registerStream(source, stream);
}

void SVRs_Stream_detachSource(SVRs_Stream* stream) {
    if(stream->source) {
        SVRs_Source_unregisterStream(stream->source, stream);
    }

    stream->source = NULL;
    stream->frame_properties = NULL;
}

void SVRs_Stream_setEncoding(SVRs_Stream* stream, SVR_Encoding* encoding) {
    stream->encoding = encoding;
}

void SVRs_Stream_addFrameFilter(SVRs_Stream* stream, SVRs_FrameFilter* frame_filter) {
    SVR_LOCK(stream);
    List_append(stream->frame_filters, frame_filter);
    stream->frame_properties = frame_filter->output_properties;
    SVR_UNLOCK(stream);
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
    SVRs_FrameFilter* filter;

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

    while((filter = List_remove(stream->frame_filters, 0)) != NULL) {
        SVRs_FrameFilter_destroy(filter);
    }
    List_destroy(stream->frame_filters);

    SVR_UNREF(stream->client);
    SVR_UNLOCK(stream);
    free(stream);
}

void SVRs_Stream_inputSourceFrame(SVRs_Stream* stream, IplImage* frame) {
    SVR_Message* message;
    SVRs_FrameFilter* frame_filter;

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

    /* Apply the frame filters */
    for(int i = 0; (frame_filter = List_get(stream->frame_filters, i)) != NULL; i++) {
        frame = SVRs_FrameFilter_apply(frame_filter, frame);
    }

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
