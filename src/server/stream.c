
#include <svr.h>
#include <svr/server/svr.h>

SVRs_Stream* SVRs_Stream_new(SVRs_Client* client, SVRs_Source* source, const char* name) {
    SVRs_Stream* stream = malloc(sizeof(SVRs_Stream));

    stream->client = client;
    stream->source = source;
    stream->name = strdup(name);
    stream->state = SVR_PAUSED;

    stream->frame_properties = source->frame_properties;
    stream->encoder = SVR_Encoder_new(source->decoder->encoding, source->frame_properties);
    stream->reencoder = SVRs_Reencoder_new(source, stream);

    stream->payload_buffer_size = 1024;
    stream->payload_buffer = malloc(stream->payload_buffer_size);

    SVR_LOCKABLE_INIT(stream);
    SVRs_Source_registerStream(source, stream);

    return stream;
}

void SVRs_Stream_destroy(SVRs_Stream* stream) {
    if(stream->name) {
        free(stream->name);
    }

    SVR_CRASH("Not implemented");
}

void SVRs_Stream_setEncoding(SVRs_Stream* stream, SVR_Encoding* encoding) {
    if(stream->encoder) {
        SVR_Encoder_destroy(stream->encoder);
    }

    stream->encoder = SVR_Encoder_new(encoding, stream->frame_properties);

    /* TODO: Reallocate reencoder */
    SVR_CRASH("Not implemented");
}

void SVRs_Stream_inputSourceData(SVRs_Stream* stream, void* data, size_t data_available) {
    SVR_Message* message;

    SVR_LOCK(stream);
    
    /* Ignore data if paused */
    if(stream->state == SVR_PAUSED) {
        SVR_UNLOCK(stream);
        return;
    }

    /* Build the data message */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Data");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->name);
    message->payload = stream->payload_buffer;

    /* Reencode data from source encoding to stream encoding, writing data to
       the stream's output buffer */
    SVRs_Reencoder_reencode(stream->reencoder, data, data_available);

    /* Send all the encoded data out in chunks */
    while(SVR_Encoder_dataReady(stream->encoder) > 0) {
        /* Get part of payload */
        message->payload_size = SVR_Encoder_readData(stream->encoder,
                                                     message->payload,
                                                     stream->payload_buffer_size);

        /* Send message */
        SVRs_Client_sendMessage(stream->client, message);
    }

    SVR_Message_release(message);
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

void SVRs_Stream_start(SVRs_Stream* stream) {
    SVR_LOCK(stream);
    stream->state = SVR_UNPAUSED;
    SVR_UNLOCK(stream);
}

void SVRs_Stream_close(SVRs_Stream* stream) {
    SVRs_Stream_pause(stream);
    Dictionary_remove(stream->client->streams, stream->name);
    SVRs_Stream_destroy(stream);
}
