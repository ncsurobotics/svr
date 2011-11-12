
#include <svr.h>

SVR_Source* SVR_Source_new(const char* name) {
    SVR_Source* source;
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.open");
    message->components[1] = SVR_Arena_strdup(message->alloc, "client");
    message->components[2] = SVR_Arena_strdup(message->alloc, name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code != SVR_SUCCESS) {
        return NULL;
    }

    source = malloc(sizeof(SVR_Source));
    source->name = strdup(name);
    source->encoding = NULL;
    source->encoding_options = NULL;
    source->encoder = NULL;
    source->frame_properties = NULL;

    source->payload_buffer_size = 4 * 1024;
    source->payload_buffer = malloc(source->payload_buffer_size);

    /* Attempt to set encoding to jpeg and try raw if that fails */
    if(SVR_Source_setEncoding(source, "jpeg") != SVR_SUCCESS) {
        SVR_Source_setEncoding(source, "raw");
    }

    return source;
}

int SVR_Source_destroy(SVR_Source* source) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.close");
    message->components[1] = SVR_Arena_strdup(message->alloc, source->name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    free(source->name);

    if(source->encoder) {
        SVR_Encoder_destroy(source->encoder);
    }

    if(source->frame_properties) {
        SVR_FrameProperties_destroy(source->frame_properties);
    }

    if(source->payload_buffer) {
        free(source->payload_buffer);
    }

    free(source);

    return return_code;
}

int SVR_Source_setEncoding(SVR_Source* source, const char* encoding_descriptor) {
    Dictionary* options;
    SVR_Encoding* encoding;
    SVR_Message* message;
    SVR_Message* response;
    int return_code;
    int err;
    
    options = SVR_parseOptionString(encoding_descriptor);
    if(options == NULL) {
        err = SVR_getOptionStringErrorPosition();
        SVR_log(SVR_DEBUG, Util_format("Parse error in \"%s\" at position %d '%c'",
                                       encoding_descriptor, err, encoding_descriptor[err]));
        return SVR_PARSEERROR;
    }
    
    encoding = SVR_Encoding_getByName(Dictionary_get(options, "%name"));
    if(encoding == NULL) {
        SVR_freeParsedOptionString(options);
        return SVR_NOSUCHENCODING;
    }

    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.setEncoding");
    message->components[1] = SVR_Arena_strdup(message->alloc, source->name);
    message->components[2] = SVR_Arena_strdup(message->alloc, encoding_descriptor);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code == SVR_SUCCESS) {
        if(source->encoding_options) {
            SVR_freeParsedOptionString(source->encoding_options);
        }
        source->encoding = encoding;
        source->encoding_options = options;
    } else {
        SVR_freeParsedOptionString(options);
    }

    return return_code;
}

int SVR_Source_setFrameProperties(SVR_Source* source, SVR_FrameProperties* frame_properties) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    message = SVR_Message_new(3);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.setFrameProperties");
    message->components[1] = SVR_Arena_strdup(message->alloc, source->name);
    message->components[2] = SVR_Arena_sprintf(message->alloc, "%d,%d,%d,%d", frame_properties->width,
                                                                               frame_properties->height,
                                                                               frame_properties->depth,
                                                                               frame_properties->channels);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    if(return_code == SVR_SUCCESS) {
        if(source->frame_properties) {
            SVR_FrameProperties_destroy(source->frame_properties);
        }

        source->frame_properties = SVR_FrameProperties_clone(frame_properties);
    }

    return return_code;
}

int SVR_Source_sendFrame(SVR_Source* source, IplImage* frame) {
    SVR_FrameProperties* frame_properties;
    SVR_Message* message;
    int return_code;

    if(source->encoding == NULL) {
        return SVR_INVALIDSTATE;
    }

    /* Automatically determine frame properties to use from the given frame */
    if(source->frame_properties == NULL) {
        frame_properties = SVR_FrameProperties_new();
        frame_properties->width = frame->width;
        frame_properties->height = frame->height;
        frame_properties->depth = frame->depth;
        frame_properties->channels = frame->nChannels;

        return_code = SVR_Source_setFrameProperties(source, frame_properties);
        SVR_FrameProperties_destroy(frame_properties);

        if(return_code != SVR_SUCCESS) {
            return return_code;
        }
    }

    if(source->encoder == NULL) {
        source->encoder = SVR_Encoder_new(source->encoding, source->encoding_options, source->frame_properties);
    }

    /* Check frame properties */
    if(frame->width != source->frame_properties->width ||
       frame->height != source->frame_properties->height ||
       frame->depth != source->frame_properties->depth ||
       frame->nChannels != source->frame_properties->channels) {

        SVR_log(SVR_WARNING, "Frame size changed!");
        return SVR_INVALIDARGUMENT;
    }

    SVR_Encoder_encode(source->encoder, frame);

    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Data");
    message->components[1] = SVR_Arena_strdup(message->alloc, source->name);
    message->payload = source->payload_buffer;

    while(SVR_Encoder_dataReady(source->encoder) > 0) {
        message->payload_size = SVR_Encoder_readData(source->encoder, message->payload, source->payload_buffer_size);
        SVR_Comm_sendMessage(message, false);
    }

    SVR_Message_release(message);
    return SVR_SUCCESS;
}

int SVR_openServerSource(const char* name, const char* descriptor) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    message = SVR_Message_new(4);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.open");
    message->components[1] = SVR_Arena_strdup(message->alloc, "server");
    message->components[2] = SVR_Arena_strdup(message->alloc, name);
    message->components[3] = SVR_Arena_strdup(message->alloc, descriptor);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
}

int SVR_closeServerSource(const char* name) {
    SVR_Message* message;
    SVR_Message* response;
    int return_code;

    message = SVR_Message_new(2);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.close");
    message->components[1] = SVR_Arena_strdup(message->alloc, name);

    response = SVR_Comm_sendMessage(message, true);
    return_code = SVR_Comm_parseResponse(response);

    SVR_Message_release(message);
    SVR_Message_release(response);

    return return_code;
}

List* SVR_getSourcesList(void) {
    List* sources_list;
    SVR_Message* message;
    SVR_Message* response;

    message = SVR_Message_new(1);
    message->components[0] = SVR_Arena_strdup(message->alloc, "Source.getSourcesList");

    response = SVR_Comm_sendMessage(message, true);

    sources_list = List_new();
    for(int i = 1; i < response->count; i++) {
        List_append(sources_list, strdup(response->components[i]));
    }

    SVR_Message_release(message);
    SVR_Message_release(response);

    return sources_list;
}

void SVR_freeSourcesList(List* sources_list) {
    char* source_name;

    for(int i = 0; (source_name = List_get(sources_list, i)) != NULL; i++) {
        free(source_name);
    }

    List_destroy(sources_list);
}
