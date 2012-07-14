
#include <svr.h>
#include <svrd.h>

/* source_name stream_name [encoding] */
void SVRD_Stream_rOpen(SVRD_Client* client, SVR_Message* message) {
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    if(SVRD_Client_getStream(client, stream_name)) {
        SVRD_Client_replyCode(client, message, SVR_NAMECLASH);
        return;
    }

    SVRD_Client_openStream(client, stream_name);
    SVRD_Client_replyCode(client, message, SVR_SUCCESS);
}

/* stream_name */
void SVRD_Stream_rClose(SVRD_Client* client, SVR_Message* message) {
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    if(SVRD_Client_getStream(client, stream_name) == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Client_closeStream(client, stream_name);
    SVRD_Client_replyCode(client, message, SVR_SUCCESS);
}

/* stream_name */
void SVRD_Stream_rGetInfo(SVRD_Client* client, SVR_Message* message) {
    SVR_Message* response;
    SVRD_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    response = SVR_Message_new(4);
    response->components[0] = SVR_Arena_strdup(response->alloc, "Stream.getInfo");

    if(stream->source) {
        response->components[1] = SVR_Arena_strdup(response->alloc, stream->source->name);
    } else {
        response->components[1] = SVR_Arena_strdup(response->alloc, "");
    }

    response->components[2] = SVR_Arena_strdup(response->alloc, stream->encoding->name);
    response->components[3] = SVR_Arena_sprintf(response->alloc, "%d,%d,%d,%d", stream->frame_properties->width,
                                                                                stream->frame_properties->height,
                                                                                stream->frame_properties->depth,
                                                                                stream->frame_properties->channels);

    SVRD_Client_reply(client, message, response);
    SVR_Message_release(response);
}

/* stream_name */
void SVRD_Stream_rPause(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Stream_pause(stream);
    SVRD_Client_replyCode(client, message, SVR_SUCCESS);
}

/* stream_name */
void SVRD_Stream_rUnpause(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Stream_unpause(stream);
    SVRD_Client_replyCode(client, message, SVR_SUCCESS);
}

void SVRD_Stream_rResize(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;
    int width, height;

    switch(message->count) {
    case 4:
        stream_name = message->components[1];
        width = atoi(message->components[2]);
        height = atoi(message->components[3]);
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Client_replyCode(client, message, SVRD_Stream_resize(stream, width, height));
}

void SVRD_Stream_rSetChannels(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;
    int channels;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        channels = atoi(message->components[2]);
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Client_replyCode(client, message, SVRD_Stream_setChannels(stream, channels));
}

void SVRD_Stream_rAttachSource(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    SVRD_Source* source;
    char* stream_name;
    char* source_name;
    int err;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        source_name = message->components[2];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    source = SVRD_Source_getByName(source_name);
    if(source == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    err = SVRD_Stream_attachSource(stream, source);
    if(err != SVR_SUCCESS) {
        /* Release source */
        SVR_UNREF(source);
    }

    SVRD_Client_replyCode(client, message, err);
}

void SVRD_Stream_rSetPriority(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;
    short priority;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        priority = atoi(message->components[2]);
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Client_replyCode(client, message, SVRD_Stream_setPriority(stream, priority));
}

void SVRD_Stream_rSetDropRate(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;
    int drop_rate;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        drop_rate = atoi(message->components[2]);
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Client_replyCode(client, message, SVRD_Stream_setDropRate(stream, drop_rate));
}

void SVRD_Stream_rSetEncoding(SVRD_Client* client, SVR_Message* message) {
    SVRD_Stream* stream;
    char* stream_name;
    char* encoding_descriptor;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        encoding_descriptor = message->components[2];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRD_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRD_Client_replyCode(client, message, SVRD_Stream_setEncoding(stream, encoding_descriptor));
}

void SVRD_Source_rOpen(SVRD_Client* client, SVR_Message* message) {
    SVRD_Source* source;
    bool client_source;
    char* source_name;
    char* source_descriptor;
    int err;

    switch(message->count) {
    case 3:
        if(strcmp(message->components[1], "client") != 0) {
            SVRD_Client_replyCode(client, message, SVR_INVALIDARGUMENT);
            return;
        }
        client_source = true;
        source_name = message->components[2];
        break;

    case 4:
        if(strcmp(message->components[1], "server") != 0) {
            SVRD_Client_replyCode(client, message, SVR_INVALIDARGUMENT);
            return;
        }
        client_source = false;
        source_name = message->components[2];
        source_descriptor = message->components[3];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    if(SVRD_Source_exists(source_name)) {
        SVRD_Client_replyCode(client, message, SVR_NAMECLASH);
        return;
    }

    if(client_source) {
        source = SVRD_Source_new(source_name);

        if(source == NULL) {
            SVRD_Client_replyCode(client, message, SVR_NAMECLASH);
            return;
        }

        SVRD_Source_setEncoding(source, "jpeg");
        SVRD_Client_provideSource(client, source);
    } else {
        source = SVRD_Source_openInstance(source_name, source_descriptor, &err);

        if(source == NULL) {
            SVRD_Client_replyCode(client, message, err);
            return;
        }
    }

    SVRD_Client_replyCode(client, message, SVR_SUCCESS);
}

void SVRD_Source_rSetEncoding(SVRD_Client* client, SVR_Message* message) {
    SVRD_Source* source;
    char* source_name;
    char* encoding_descriptor;

    switch(message->count) {
    case 3:
        source_name = message->components[1];
        encoding_descriptor = message->components[2];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRD_Client_getSource(client, source_name);
    if(source == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    SVRD_Client_replyCode(client, message, SVRD_Source_setEncoding(source, encoding_descriptor));
}

void SVRD_Source_rSetFrameProperties(SVRD_Client* client, SVR_Message* message) {
    SVRD_Source* source;
    SVR_FrameProperties* frame_properties;
    char* source_name;
    char* frame_properties_string;
    int return_code;

    switch(message->count) {
    case 3:
        source_name = message->components[1];
        frame_properties_string = message->components[2];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRD_Client_getSource(client, source_name);
    if(source == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    frame_properties = SVR_FrameProperties_fromString(frame_properties_string);
    return_code = SVRD_Source_setFrameProperties(source, frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    SVRD_Client_replyCode(client, message, return_code);
}

void SVRD_Source_rData(SVRD_Client* client, SVR_Message* message) {
    SVRD_Source* source;
    char* source_name;

    switch(message->count) {
    case 2:
        source_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRD_Client_getSource(client, source_name);
    if(source == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    SVRD_Source_provideData(source, message->payload, message->payload_size);
}

void SVRD_Source_rClose(SVRD_Client* client, SVR_Message* message) {
    SVRD_Source* source;
    char* source_name;

    switch(message->count) {
    case 2:
        source_name = message->components[1];
        break;

    default:
        SVRD_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRD_Source_getByName(source_name);
    if(source == NULL) {
        SVRD_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    /* Client source, verify it belongs to this client */
    if(source->type == NULL) {
        if(SVRD_Client_getSource(client, source_name) == NULL) {
            SVRD_Client_replyCode(client, message, SVR_INVALIDARGUMENT);
            SVR_UNREF(source);
            return;
        }

        SVRD_Client_unprovideSource(client, source);
    }

    SVRD_Source_destroy(source);
    SVR_UNREF(source);

    SVRD_Client_replyCode(client, message, SVR_SUCCESS);
}

void SVRD_Source_rGetSourcesList(SVRD_Client* client, SVR_Message* message) {
    SVR_Message* response;
    SVRD_Source* source;
    List* sources_list;
    char* source_name;
    int j;

    if(message->count != 1) {
        return;
    }

    sources_list = SVRD_Source_getSourcesList();
    response = SVR_Message_new(List_getSize(sources_list) + 1);

    response->components[0] = SVR_Arena_strdup(response->alloc, "Source.getSourceList");
    j = 1;
    for(int i = 0; (source_name = List_get(sources_list, i)) != NULL; i++) {
        source = SVRD_Source_getLockedSource(source_name);
        free(source_name);

        if(source == NULL) {
            message->count--;
            continue;
        }

        if(source->type == NULL) {
            response->components[j] = SVR_Arena_sprintf(response->alloc, "c:%s", source->name);
        } else {
            response->components[j] = SVR_Arena_sprintf(response->alloc, "s:%s", source->name);
        }

        SVR_UNLOCK(source);
        SVR_UNREF(source);
        j++;
    }

    List_destroy(sources_list);
    SVRD_Client_reply(client, message, response);
    SVR_Message_release(response);
}

void SVRD_Event_rRegister(SVRD_Client* client, SVR_Message* message) {
    // --
}

void SVRD_Event_rUnregister(SVRD_Client* client, SVR_Message* message) {
    // --
}
