
#include <svr.h>
#include <svr/server/svr.h>

/* source_name stream_name [encoding] */
void SVRs_Stream_rOpen(SVRs_Client* client, SVR_Message* message) {
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    if(SVRs_Client_getStream(client, stream_name)) {
        SVRs_Client_replyCode(client, message, SVR_NAMECLASH);
        return;
    }

    SVRs_Client_openStream(client, stream_name);
    SVRs_Client_replyCode(client, message, SVR_SUCCESS);
}

/* stream_name */
void SVRs_Stream_rClose(SVRs_Client* client, SVR_Message* message) {
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    if(SVRs_Client_getStream(client, stream_name) == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Client_closeStream(client, stream_name);
    SVRs_Client_replyCode(client, message, SVR_SUCCESS);
}

/* stream_name */
void SVRs_Stream_rGetInfo(SVRs_Client* client, SVR_Message* message) {
    SVR_Message* response;
    SVRs_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
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

    SVRs_Client_reply(client, message, response);
    SVR_Message_release(response);
}

/* stream_name */
void SVRs_Stream_rPause(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Stream_pause(stream);
    SVRs_Client_replyCode(client, message, SVR_SUCCESS);
}

/* stream_name */
void SVRs_Stream_rUnpause(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Stream_unpause(stream);
    SVRs_Client_replyCode(client, message, SVR_SUCCESS);
}

void SVRs_Stream_rResize(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    char* stream_name;
    int width, height;

    switch(message->count) {
    case 4:
        stream_name = message->components[1];
        width = atoi(message->components[2]);
        height = atoi(message->components[3]);
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Client_replyCode(client, message, SVRs_Stream_resize(stream, width, height));
}

void SVRs_Stream_rSetChannels(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    char* stream_name;
    int channels;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        channels = atoi(message->components[2]);
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Client_replyCode(client, message, SVRs_Stream_setChannels(stream, channels));
}

void SVRs_Stream_rAttachSource(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    SVRs_Source* source;
    char* stream_name;
    char* source_name;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        source_name = message->components[2];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    source = SVRs_Source_getByName(source_name);
    if(source == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    SVRs_Client_replyCode(client, message, SVRs_Stream_attachSource(stream, source));
}

void SVRs_Stream_rSetDropRate(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    char* stream_name;
    int drop_rate;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        drop_rate = atoi(message->components[2]);
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Client_replyCode(client, message, SVRs_Stream_setDropRate(stream, drop_rate));
}

void SVRs_Stream_rSetEncoding(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    SVR_Encoding* encoding;
    char* stream_name;
    char* encoding_name;

    switch(message->count) {
    case 3:
        stream_name = message->components[1];
        encoding_name = message->components[2];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = SVRs_Client_getStream(client, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    encoding = SVR_Encoding_getByName(encoding_name);
    if(encoding == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHENCODING);
        return;
    }

    SVRs_Client_replyCode(client, message, SVRs_Stream_setEncoding(stream, encoding));
}

void SVRs_Source_rOpen(SVRs_Client* client, SVR_Message* message) {
    SVRs_Source* source;
    bool client_source;
    char* source_name;
    char* source_descriptor;

    switch(message->count) {
    case 3:
        if(strcmp(message->components[1], "client") != 0) {
            SVRs_Client_replyCode(client, message, SVR_INVALIDARGUMENT);
            return;
        }
        client_source = true;
        source_name = message->components[2];
        break;

    case 4:
        if(strcmp(message->components[1], "server") != 0) {
            SVRs_Client_replyCode(client, message, SVR_INVALIDARGUMENT);
            return;
        }
        client_source = false;
        source_name = message->components[2];
        source_descriptor = message->components[3];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    if(SVRs_Source_getByName(source_name)) {
        SVRs_Client_replyCode(client, message, SVR_NAMECLASH);
        return;
    }

    if(client_source) {
        source = SVRs_Source_new(source_name);

        if(source == NULL) {
            SVRs_Client_replyCode(client, message, SVR_NAMECLASH);
            return;
        }

        SVRs_Source_setEncoding(source, SVR_Encoding_getByName("jpeg"));
        SVRs_Client_provideSource(client, source);
    } else {
        source = SVRs_Source_openInstance(source_name, source_descriptor);

        if(source == NULL) {
            SVRs_Client_replyCode(client, message, SVR_UNKNOWNERROR);
            return;
        }
    }

    SVRs_Client_replyCode(client, message, SVR_SUCCESS);
}

void SVRs_Source_rSetEncoding(SVRs_Client* client, SVR_Message* message) {
    SVRs_Source* source;
    SVR_Encoding* encoding;
    char* source_name;
    char* encoding_name;

    switch(message->count) {
    case 3:
        source_name = message->components[1];
        encoding_name = message->components[2];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRs_Client_getSource(client, source_name);
    if(source == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    encoding = SVR_Encoding_getByName(encoding_name);
    if(encoding == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHENCODING);
        return;
    }

    SVRs_Client_replyCode(client, message, SVRs_Source_setEncoding(source, encoding));
}

void SVRs_Source_rSetFrameProperties(SVRs_Client* client, SVR_Message* message) {
    SVRs_Source* source;
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
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRs_Client_getSource(client, source_name);
    if(source == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    frame_properties = SVR_FrameProperties_fromString(frame_properties_string);
    return_code = SVRs_Source_setFrameProperties(source, frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    SVRs_Client_replyCode(client, message, return_code);
}

void SVRs_Source_rData(SVRs_Client* client, SVR_Message* message) {
    SVRs_Source* source;
    char* source_name;

    switch(message->count) {
    case 2:
        source_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRs_Client_getSource(client, source_name);
    if(source == NULL) {
        SVRs_Client_replyCode(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    SVRs_Source_provideData(source, message->payload, message->payload_size);
}

void SVRs_Source_rClose(SVRs_Client* client, SVR_Message* message) {
    SVR_CRASH("Not implemented");
}

void SVRs_Event_rRegister(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Event_rUnregister(SVRs_Client* client, SVR_Message* message) {
    // --
}
