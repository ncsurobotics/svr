/**
 * \file
 * \brief Streams
 */

#include <svr.h>

static SVR_Stream* SVR_Stream_getByName(const char* stream_name);
static int SVR_Stream_updateInfo(SVR_Stream* stream);
static int SVR_Stream_open(SVR_Stream* stream);
static int SVR_Stream_close(SVR_Stream* stream);

static pthread_mutex_t stream_list_lock = PTHREAD_MUTEX_INITIALIZER;
static Dictionary* streams;
static unsigned int last_stream_num = 0;

static pthread_mutex_t new_global_data_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t new_global_data_cond = PTHREAD_COND_INITIALIZER;
static bool new_global_data = false;

/**
 * \defgroup Stream Stream
 * \brief A stream provides access to a source available through a running SVR server
 * \{
 */

/**
 * \brief Initialize the stream component
 *
 * Initialize the stream component
 */
void SVR_Stream_init(void) {
    streams = Dictionary_new();
}

/**
 * Get a stream by name
 */
static SVR_Stream* SVR_Stream_getByName(const char* stream_name) {
    return Dictionary_get(streams, stream_name);
}

/**
 * \brief Create a new stream
 *
 * Create a new stream with the source given by source_name. The source is
 * intially paused.
 *
 * \param source_name The name of the source which the steam should be created
 * for
 * \return The new stream
 */
SVR_Stream* SVR_Stream_new(const char* source_name) {
    SVR_Stream* stream = malloc(sizeof(SVR_Stream));

    stream->stream_name = strdup(Util_format("stream%u", last_stream_num++));
    stream->source_name = strdup(source_name);
    stream->state = SVR_PAUSED;
    stream->current_frame = NULL;
    stream->frame_properties = NULL;
    stream->encoding = NULL;
    stream->decoder = NULL;
    stream->orphaned = false;

    pthread_cond_init(&stream->new_frame, NULL);
    SVR_LOCKABLE_INIT(stream);

    /* Communicate with server to open the stream */
    if(SVR_Stream_open(stream) != SVR_SUCCESS) {
        free(stream);
        return NULL;
    }

    return stream;
}

/**
 * \brief Destroy a stream
 *
 * Close and destroy a stream
 *
 * \param stream The stream to close
 */
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

    free(stream->stream_name);
    free(stream->source_name);

    SVR_UNLOCK(stream);
    free(stream);
}

/**
 * Open a stream with the server
 */
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

/**
 * Close a stream with the server
 */
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

/**
 * Update the stream info from the server
 */
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

/**
 * \brief Set the stream encoding
 *
 * Set the encoding of the stream. The stream must be paused to set the
 * encoding.
 *
 * \param stream The stream
 * \param encoding_descriptor Option string describing the new encoding
 * \return An SVR return code
 */
int SVR_Stream_setEncoding(SVR_Stream* stream, const char* encoding_descriptor) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.setEncoding");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    message->components[2] = SVR_Arena_strdup(message->alloc, encoding_descriptor);

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

/**
 * \brief Resize the stream
 *
 * Request the stream be resized on the server side being being sent
 *
 * \param stream The stream
 * \param width The new width
 * \param height The new height
 * \return An SVR return code
 */
int SVR_Stream_resize(SVR_Stream* stream, int width, int height) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

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

/**
 * \brief Change the color mode of the stream
 *
 * Set/unset the grayscale setting for the stream
 *
 * \param stream The stream
 * \param grayscale Grayscale flag for the stream
 * \return An SVR return code
 */
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

/**
 * \brief Set the priority of the stream
 *
 * Set the stream priority. This currently has not effect
 *
 * \param stream The stream
 * \param priority The stream priority
 * \return An SVR return code
 */
int SVR_Stream_setPriority(SVR_Stream* stream, short priority) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.setPriority");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    message->components[2] = SVR_Arena_sprintf(message->alloc, "%d", (int) priority);

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

/**
 * \brief Set the stream droprate
 *
 * Set the drop rate for the stream. A drop rate of n specifies that one of
 * every n frames should be sent. Thus a drop rate of 1 is full rate, 2 is half
 * rate, 3 third, etc.
 *
 * \param stream The stream
 * \param drop_rate The drop rate
 * \return An SVR return code
 */
int SVR_Stream_setDropRate(SVR_Stream* stream, int drop_rate) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    /* Open stream */
    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Stream.setDropRate");
    message->components[1] = SVR_Arena_strdup(message->alloc, stream->stream_name);
    message->components[2] = SVR_Arena_sprintf(message->alloc, "%d", drop_rate);

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

/**
 * \brief Unpause the stream
 *
 * Unpause a stream. A stream must be unpaused to receive frames
 *
 * \param stream The stream
 * \return An SVR return code
 */
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

    if(return_code == 0) {
        stream->state = SVR_UNPAUSED;
    }

    return return_code;
}


/**
 * \brief Pause the stream
 *
 * Pause a stream.
 *
 * \param stream The stream
 * \return An SVR return code
 */
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

    if(return_code == 0) {
        stream->state = SVR_PAUSED;
    }

    return return_code;
}

/**
 * \brief Get the stream frame properties
 *
 * Get the frame properties for the stream
 *
 * \param stream The stream
 * \return The stream's frame properties
 */
SVR_FrameProperties* SVR_Stream_getFrameProperties(SVR_Stream* stream) {
    return stream->frame_properties;
}

/**
 * \brief Get a frame from a stream
 *
 * Get a frame from a stream. The most recent frame is always returned
 * (i.e. missed frame are silently dropped). When the caller is finished with
 * a frame it should be returned with a call to SVR_Stream_returnFrame
 *
 * \param stream The stream
 * \param wait A wait flag. If true, block until a frame is available. Otherwise
 * return NULL immediately if a frame is unavailable.
 * \return A frame, or NULL if wait was false and no frame was available
 */
IplImage* SVR_Stream_getFrame(SVR_Stream* stream, bool wait) {
    IplImage* frame;

    SVR_LOCK(stream);
    while(stream->current_frame == NULL && wait && stream->state == SVR_UNPAUSED) {
        SVR_LOCK_WAIT(stream, &stream->new_frame);
    }

    frame = stream->current_frame;
    stream->current_frame = NULL;
    SVR_UNLOCK(stream);

    return frame;
}

/**
 * \brief Return a frame
 *
 * When the caller is done with a frame returned by SVR_Stream_getFrame it
 * should be retuned via this function.
 *
 * \param stream The stream
 * \param frame The frame to return
 */
void SVR_Stream_returnFrame(SVR_Stream* stream, IplImage* frame) {
    if(stream->decoder) {
        SVR_Decoder_returnFrame(stream->decoder, frame);
    }
}

/**
 * \brief Check stream orphaned status
 *
 * Check if a stream has been orphaned. A stream is orphaned when its source
 * closes.
 *
 * \param stream The stream
 * \return True if the stream has been orphaned, false otherwise
 */
bool SVR_Stream_isOrphaned(SVR_Stream* stream) {
    return stream->orphaned;
}

/**
 * \private
 * \brief Mark the stream as orphaned
 *
 * Mark the stream as orphaned
 *
 * \param stream_name The name of the stream to mark as orphaned
 */
void SVR_Stream_setOrphaned(const char* stream_name) {
    SVR_Stream* stream;

    pthread_mutex_lock(&stream_list_lock);
    stream = SVR_Stream_getByName(stream_name);
    if(stream == NULL) {
        SVR_log(SVR_WARNING, "Received orphaned signal for uknown stream");
        return;
    }
    SVR_LOCK(stream);
    pthread_mutex_unlock(&stream_list_lock);

    /* We've been orphaned, pause the stream, mark as orphaned and wake up any
       getFrame calls */
    stream->orphaned = true;
    stream->state = SVR_PAUSED;
    pthread_cond_broadcast(&stream->new_frame);
    SVR_UNLOCK(stream);

    /* Notify SVR_Stream_sync that something happened */
    pthread_mutex_lock(&new_global_data_lock);
    new_global_data = true;
    pthread_cond_broadcast(&new_global_data_cond);
    pthread_mutex_unlock(&new_global_data_lock);
}

/**
 * \brief Block until a frame is ready
 *
 * Block until a frame is available from any open stream
 */
void SVR_Stream_sync(void) {
    pthread_mutex_lock(&new_global_data_lock);
    if(new_global_data == false) {
        pthread_cond_wait(&new_global_data_cond, &new_global_data_lock);
    }
    new_global_data = false;
    pthread_mutex_unlock(&new_global_data_lock);
}

/**
 * \private
 * \brief Provide encoded source data to a stream
 *
 * Provide encoded source data to a stream
 *
 * \param stream_name Name of the stream the data is for
 * \param buffer A buffer of encoded frame data
 * \param n Number of bytes in the buffer
 */
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

    pthread_mutex_lock(&new_global_data_lock);
    new_global_data = true;
    pthread_cond_broadcast(&new_global_data_cond);
    pthread_mutex_unlock(&new_global_data_lock);

    SVR_UNLOCK(stream);
}

/** \} */

