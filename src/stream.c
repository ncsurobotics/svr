
#include <svr.h>

static Dictionary* streams;

void SVR_Stream_init(void) {
    streams = Dictionary_new();
}

SVR_Stream* SVR_Stream_new(const char* stream_name, const char* source_name) {
    SVR_Stream* stream = malloc(sizeof(SVR_Stream));
    SVR_FrameProperties* properties = SVR_FrameProperties_new();

    properties->width = 320;
    properties->height = 240;
    properties->channels = 1;
    properties->depth = 8;

    stream->stream_name = stream_name;
    stream->source_name = source_name;
    stream->current_frame = NULL;
    stream->decoder = SVR_Decoder_new(SVR_Encoding_getByName("raw"), properties);
    pthread_cond_init(&stream->new_frame, NULL);
    SVR_LOCKABLE_INIT(stream);

    SVR_FrameProperties_destroy(properties);

    return stream;
}

int SVR_Stream_open(SVR_Stream* stream) {
    SVR_Message* message = SVR_Message_new(3);
    SVR_Message* response;
    int return_code;

    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.open");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->source_name);
    message->components[2] = SVR_Arena_strdup(message->alloc, stream->stream_name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
}

static SVR_Stream* SVR_Stream_getByName(const char* stream_name) {
    return Dictionary_get(streams, stream_name);
}

IplImage* SVR_Stream_getFrame(SVR_Stream* stream, bool wait) {
    IplImage* frame;

    SVR_LOCK(stream);
    while(stream->current_frame == NULL && wait) {
        SVR_LOCK_WAIT(stream, &stream->new_frame);
    }

    frame = stream->current_frame;
    stream->current_frame = NULL;
    SVR_UNLOCK(stream);

    return frame;
}

void SVR_Stream_returnFrame(SVR_Stream* stream, IplImage* frame) {
    SVR_Decoder_returnFrame(stream->decoder, frame);
}

void SVR_Stream_provideData(const char* stream_name, void* buffer, size_t n) {
    SVR_Stream* stream = SVR_Stream_getByName(stream_name);

    SVR_LOCK(stream);
    SVR_Decoder_decode(stream->decoder, buffer, n);

    if(SVR_Decoder_framesReady(stream->decoder)) {
        if(stream->current_frame) {
            SVR_Decoder_returnFrame(stream->decoder, stream->current_frame);
        }

        while(SVR_Decoder_framesReady(stream->decoder) > 1) {
            SVR_Decoder_returnFrame(stream->decoder, SVR_Decoder_getFrame(stream->decoder));
        }

        stream->current_frame = SVR_Decoder_getFrame(stream->decoder);
        pthread_cond_broadcast(&stream->new_frame);
    }

    SVR_UNLOCK(stream);
}
