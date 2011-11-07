
#include <svr.h>

static SVR_Stream* SVR_Stream_getByName(const char* stream_name);
static int SVR_Stream_updateInfo(SVR_Stream* stream);
static int SVR_Stream_open(SVR_Stream* stream);
static int SVR_Stream_close(SVR_Stream* stream);

static pthread_mutex_t stream_list_lock = PTHREAD_MUTEX_INITIALIZER;

static Dictionary* streams;

void SVR_Stream_init(void) {
    streams = Dictionary_new();
}

static SVR_Stream* SVR_Stream_getByName(const char* stream_name) {
    return Dictionary_get(streams, stream_name);
}

SVR_Stream* SVR_Stream_new(const char* stream_name, const char* source_name) {
    SVR_Stream* stream = malloc(sizeof(SVR_Stream));

    stream->stream_name = stream_name;
    stream->source_name = source_name;
    stream->current_frame = NULL;
    stream->frame_properties = NULL;
    stream->encoding = NULL;
    stream->decoder = NULL;

    pthread_cond_init(&stream->new_frame, NULL);
    SVR_LOCKABLE_INIT(stream);

    /* Communicate with server to open the stream */
    if(SVR_Stream_open(stream) != SVR_SUCCESS) {
        free(stream);
        return NULL;
    }

    return stream;
}

void SVR_Stream_destroy(SVR_Stream* stream) {
    SVR_Stream_close(stream);

    pthread_mutex_lock(&stream_list_lock);
    Dictionary_remove(streams, stream->stream_name);
    SVR_LOCK(stream);
    pthread_mutex_unlock(&stream_list_lock);

    if(stream->frame_properties) {
        SVR_FrameProperties_destroy(stream->frame_properties);
    }

    if(stream->decoder) {
        if(stream->current_frame) {
            SVR_Decoder_returnFrame(stream->decoder, stream->current_frame);
        }

        SVR_Decoder_destroy(stream->decoder);
    }

    SVR_UNLOCK(stream);
    free(stream);
}

static int SVR_Stream_open(SVR_Stream* stream) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.open");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code != SVR_SUCCESS) {
        return return_code;
    }

    /* Attach source to stream */
    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.attachSource");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    message->components[2] = SVR_Arena_strdup(message->alloc, stream->source_name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code != SVR_SUCCESS) {
        return return_code;
    }

    return_code = SVR_Stream_updateInfo(stream);
    if(return_code != SVR_SUCCESS) {
        return return_code;
    }

    /* Save stream */
    pthread_mutex_lock(&stream_list_lock);
    Dictionary_set(streams, stream->stream_name, stream);
    pthread_mutex_unlock(&stream_list_lock);

    return return_code;
}

static int SVR_Stream_close(SVR_Stream* stream) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Close stream */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.close");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
}

static int SVR_Stream_updateInfo(SVR_Stream* stream) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Get encoding and frame properties */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.getInfo");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    response = SVR_Comm_sendMessage(message, true);

    if(response->count == 4 && strcmp(response->components[0], "Stream.getInfo") == 0) {
        if(stream->frame_properties) {
            SVR_FrameProperties_destroy(stream->frame_properties);
        }

        stream->encoding = SVR_Encoding_getByName(response->components[2]);
        stream->frame_properties = SVR_FrameProperties_fromString(response->components[3]);
        return_code = 0;
    } else {
        return_code = SVR_Comm_parseResponse(response);
    }

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
}

int SVR_Stream_resize(SVR_Stream* stream, int width, int height) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(4);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.resize");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    message->components[2] = SVR_Arena_sprintf(message->alloc, "%d", width);
    message->components[3] = SVR_Arena_sprintf(message->alloc, "%d", height);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code != SVR_SUCCESS) {
        return return_code;
    }

    return_code = SVR_Stream_updateInfo(stream);

    return return_code;
}

int SVR_Stream_setGrayscale(SVR_Stream* stream, bool grayscale) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.setChannels");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    if(grayscale) {
        message->components[2] = SVR_Arena_strdup(message->alloc, "1");
    } else {
        message->components[2] = SVR_Arena_strdup(message->alloc, "0");
    }

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code != SVR_SUCCESS) {
        return return_code;
    }

    return_code = SVR_Stream_updateInfo(stream);

    return return_code;
}

int SVR_Stream_unpause(SVR_Stream* stream) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Reopen decoder */
    if(stream->decoder) {
        SVR_Decoder_destroy(stream->decoder);
    }
    stream->decoder = SVR_Decoder_new(stream->encoding, stream->frame_properties);

    /* Open stream */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.unpause");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
}

int SVR_Stream_pause(SVR_Stream* stream) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.pause");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
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
    SVR_Stream* stream;

    pthread_mutex_lock(&stream_list_lock);
    stream = SVR_Stream_getByName(stream_name);
    if(stream == NULL) {
        SVR_log(SVR_WARNING, "Data arrived for unknown stream\n");
        return;
    }
    SVR_LOCK(stream);
    pthread_mutex_unlock(&stream_list_lock);

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
