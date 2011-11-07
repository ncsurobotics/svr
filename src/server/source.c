
#include <svr.h>
#include <svr/server/svr.h>

static Dictionary* sources = NULL;

void SVRs_Source_init(void) {
    sources = Dictionary_new();
}

SVRs_Source* SVRs_Source_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties) {
    SVRs_Source* source = malloc(sizeof(SVRs_Source));

    source->decoder = SVR_Decoder_new(encoding, frame_properties);
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

SVR_FrameProperties* SVRs_Source_getFrameProperties(SVRs_Source* source) {
    return source->frame_properties;
}

SVRs_Source* SVRs_getSourceByName(const char* source_name) {
    return Dictionary_get(sources, source_name);
}

void SVRs_Source_registerStream(SVRs_Source* source, SVRs_Stream* stream) {
    SVR_LOCK(source);
    List_append(source->streams, stream);
    SVR_UNLOCK(source);
}

void SVRs_Source_unregisterStream(SVRs_Source* source, SVRs_Stream* stream) {
    SVR_LOCK(source);
    List_remove(source->streams, List_indexOf(source->streams, stream));
    SVR_UNLOCK(source);
}

void SVRs_Source_provideData(SVRs_Source* source, void* data, size_t data_available) {
    SVRs_Stream* stream;
    IplImage* frame;

    SVR_LOCK(source);

    SVR_Decoder_decode(source->decoder, data, data_available);

    while(SVR_Decoder_framesReady(source->decoder) > 0) {
        frame = SVR_Decoder_getFrame(source->decoder);
        for(int i = 0; (stream = List_get(source->streams, i)) != NULL; i++) {
            SVRs_Stream_inputSourceFrame(stream, frame);
        }
        SVR_Decoder_returnFrame(source->decoder, frame);
    }
    SVR_UNLOCK(source);
}
