
#include <svr.h>
#include <svr/server/svr.h>

SVRs_Stream* SVRs_Stream_new(SVRs_Client* client, SVRs_Source* source) {
    SVRs_Stream* stream = malloc(sizeof(SVRs_Stream));

    stream->client = client;
    stream->source = source;

    stream->frame_properties = source->frame_properties;

    stream->encoding = source->encoding;
    stream->encoder_data = stream->encoding->openEncoder(stream->frame_properties);

    stream->reencoder = SVRs_Reencoder_new(source, stream);

    stream->state = SVR_STREAM_PAUSED;

    stream->name = NULL;

    stream->buffer = stream->encoding->newOutputBuffer(stream->encoder_data, &stream->buffer_size);

    SVRs_Source_registerStream(source, stream);
    
    SVR_LOCKABLE_INIT(stream);

    return stream;
}

void SVRs_Stream_open(SVRs_Client* client, SVRs_Source* source, SVR_Encoding* encoding, const char* stream_name) {
    SVRs_Stream* stream;

    stream = SVRs_Stream_new(client, source);
    stream->name = strdup(stream_name);

    if(encoding) {
        SVRs_Stream_setEncoding(stream, encoding);
    }

    Dictionary_set(client->streams, stream_name, stream);
}

void SVRs_Stream_inputSourceData(SVRs_Stream* stream, SVR_DataBuffer* data, size_t data_available) {
    SVR_Message* message = SVR_Message_new(2);
    size_t n;

    SVR_LOCK(stream);
    n = stream->reencoder->reencode(stream->reencoder, data, data_available, stream->buffer, stream->buffer_size);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Data");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->name);
    message->payload = stream->buffer;
    message->payload_size = n;
    SVRs_Client_sendMessage(stream->client, message);
    SVR_Message_release(message);
    SVR_UNLOCK(stream);
}

void SVRs_Stream_pause(SVRs_Stream* stream) {
    // --
}

void SVRs_Stream_start(SVRs_Stream* stream) {
    // --
}

void SVRs_Stream_close(SVRs_Stream* stream) {
    SVRs_Stream_pause(stream);
    Dictionary_remove(stream->client->streams, stream->name);
    SVRs_Stream_destroy(stream);
}
