
#include <svr.h>
#include <svr/server/svr.h>

static Dictionary* sources = NULL;

void SVRs_Source_init(void) {
    sources = Dictionary_new();
}

SVRs_Source* SVRs_Source_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties) {
    SVRs_Source* source = malloc(sizeof(SVRs_Source));

    source->encoding = encoding;
    source->decoder_data = source->encoding->openDecoder(frame_properties);
    source->frame_properties = frame_properties;
    source->streams = List_new();
    source->name = NULL;

    SVR_LOCKABLE_INIT(source);

    return source;
}

SVRs_Source* SVRs_Source_open(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties, const char* name) {
    SVRs_Source* source;

    if(Dictionary_exists(sources, name)) {
        return NULL;
    }

    source = SVRs_Source_new(encoding, frame_properties);
    source->name = strdup(name);

    Dictionary_set(sources, name, source);

    return source;
}

void SVRs_Source_registerStream(SVRs_Source* source, SVRs_Stream* stream) {
    SVR_LOCK(source);
    List_append(source->streams, stream);
    SVR_UNLOCK(source);
}

void SVRs_Source_provideData(SVRs_Source* source, SVR_DataBuffer* data, size_t data_available) {
    SVRs_Stream* stream;

    SVR_LOCK(source);
    for(int i = 0; (stream = List_get(source->streams, i)) != NULL; i++) {
        SVRs_Stream_inputSourceData(stream, data, data_available);
    }
    SVR_UNLOCK(source);
}
