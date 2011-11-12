
#include <svr.h>
#include <svrd.h>

#include <ctype.h>

#include "sources/sources.h"

static void SVRD_Source_addType(SVRD_SourceType* source_type);

static Dictionary* sources = NULL;
static Dictionary* source_types = NULL;
static pthread_mutex_t sources_lock = PTHREAD_MUTEX_INITIALIZER;

void SVRD_Source_init(void) {
    sources = Dictionary_new();
    source_types = Dictionary_new();

    SVRD_Source_addType(&SVR_SOURCE(test));
    SVRD_Source_addType(&SVR_SOURCE(cam));
    SVRD_Source_addType(&SVR_SOURCE(file));
}

SVRD_Source* SVRD_Source_getByName(const char* source_name) {
    return Dictionary_get(sources, source_name);
}

SVRD_Source* SVRD_Source_openInstance(const char* source_name, const char* descriptor, int* return_code) {
    Dictionary* options;
    SVRD_Source* source;
    SVRD_SourceType* source_type;
    int err;

    options = SVR_parseOptionString(descriptor);
    if(options == NULL) {
        err = SVR_getOptionStringErrorPosition();
        SVR_log(SVR_ERROR, Util_format("Error parsing source descriptor \"%s\" at position %d, character '%c'",
                                       descriptor, err, descriptor[err]));
        if(return_code) {
            *return_code = SVR_PARSEERROR;
        }
        return NULL;
    }

    source_type = Dictionary_get(source_types, Dictionary_get(options, "%name"));
    if(source_type == NULL) {
        SVR_log(SVR_DEBUG, Util_format("No such source type '%s'", Dictionary_get(options, "%name")));
        SVR_freeParsedOptionString(options);
        if(return_code) {
            *return_code = SVR_INVALIDARGUMENT;
        }
        return NULL;
    }

    source = source_type->open(source_name, options);
    SVR_freeParsedOptionString(options);

    if(source) {
        source->type = source_type;
    }

    if(return_code) {
        if(source) {
            *return_code = SVR_SUCCESS;
        } else {
            *return_code = SVR_UNKNOWNERROR;
        }
    }

    return source;
}

void SVRD_Source_fromFile(const char* filename) {
    Dictionary* source_descriptions = Config_readFile(filename);
    List* source_names;
    char* source_name;

    if(source_descriptions == NULL) {
        switch(Config_getError()) {
        case CONFIG_EFILEACCESS:
            SVR_log(SVR_CRITICAL, Util_format("Failed to open source description file: %s", strerror(errno)));
            break;
        case CONFIG_ELINETOOLONG:
            SVR_log(SVR_CRITICAL, Util_format("Line exceeded maximum allowable length at line %d", Config_getLineNumber()));
            break;
        case CONFIG_EPARSE:
            SVR_log(SVR_CRITICAL, Util_format("Parse error occurred on line %d", Config_getLineNumber()));
            break;
        default:
            SVR_log(SVR_CRITICAL, "Unknown error occurred while reading source description file");
            break;
        }

        SVRD_exitError();
        return;
    }

    source_names = Dictionary_getKeys(source_descriptions);
    for(int i = 0; (source_name = List_get(source_names, i)) != NULL; i++) {
        if(SVRD_Source_openInstance(source_name, Dictionary_get(source_descriptions, source_name), NULL) == NULL) {
            SVR_log(SVR_CRITICAL, "Error parsing stream descriptor and/or starting stream");
            SVRD_exitError();
        }
        SVR_log(SVR_INFO, Util_format("Opened source \"%s\"", source_name));
    }

    List_destroy(source_names);
    Dictionary_destroy(source_descriptions);
}

static void SVRD_Source_addType(SVRD_SourceType* source_type) {
    Dictionary_set(source_types, source_type->name, source_type);
}

SVRD_Source* SVRD_Source_new(const char* name) {
    SVRD_Source* source;

    pthread_mutex_lock(&sources_lock);
    if(Dictionary_exists(sources, name)) {
        pthread_mutex_unlock(&sources_lock);
        return NULL;
    }

    source = malloc(sizeof(SVRD_Source));
    source->name = strdup(name);
    source->streams = List_new();
    source->frame_properties = NULL;
    source->encoding = NULL;
    source->decoder = NULL;
    source->type = NULL;
    source->private_data = NULL;
    SVR_LOCKABLE_INIT(source);

    Dictionary_set(sources, name, source);
    pthread_mutex_unlock(&sources_lock);

    return source;
}

void SVRD_Source_destroy(SVRD_Source* source) {
    SVRD_Stream* stream;

    /* Remove source from sources list and ensure source is only destroyed
       once */
    pthread_mutex_lock(&sources_lock);
    if(Dictionary_exists(sources, source->name) == false) {
        pthread_mutex_unlock(&sources_lock);
        return;
    }

    Dictionary_remove(sources, source->name);
    pthread_mutex_unlock(&sources_lock);

    if(source->type && source->type->close) {
        source->type->close(source);
    }

    SVR_LOCK(source);

    /* Send signals to streams first */
    for(int i = 0; (stream = List_get(source->streams, i)) != NULL; i++) {
        SVRD_Stream_sourceClosing(stream);
    }
    List_destroy(source->streams);

    if(source->frame_properties) {
        SVR_FrameProperties_destroy(source->frame_properties);
    }

    free(source->name);
    SVR_Decoder_destroy(source->decoder);
    SVR_UNLOCK(source);

    free(source);
}

int SVRD_Source_setEncoding(SVRD_Source* source, const char* encoding_descriptor) {
    Dictionary* options;
    SVR_Encoding* encoding;
    
    if(source->decoder) {
        /* Source already started */
        return SVR_INVALIDSTATE;
    }

    options = SVR_parseOptionString(encoding_descriptor);
    if(options == NULL) {
        return SVR_PARSEERROR;
    }

    encoding = SVR_Encoding_getByName(Dictionary_get(options, "%name"));
    if(encoding == NULL) {
        SVR_freeParsedOptionString(options);
        return SVR_NOSUCHENCODING;
    }

    source->encoding = encoding;
    SVR_freeParsedOptionString(options);
    
    return SVR_SUCCESS;
}

int SVRD_Source_setFrameProperties(SVRD_Source* source, SVR_FrameProperties* frame_properties) {
    if(source->decoder) {
        /* Source already started */
        return SVR_INVALIDSTATE;
    }

    if(source->frame_properties) {
        SVR_FrameProperties_destroy(source->frame_properties);
    }

    source->frame_properties = SVR_FrameProperties_clone(frame_properties);
    return SVR_SUCCESS;
}

SVR_FrameProperties* SVRD_Source_getFrameProperties(SVRD_Source* source) {
    return source->frame_properties;
}

void SVRD_Source_registerStream(SVRD_Source* source, SVRD_Stream* stream) {
    SVR_LOCK(source);
    List_append(source->streams, stream);
    SVR_UNLOCK(source);
}

void SVRD_Source_unregisterStream(SVRD_Source* source, SVRD_Stream* stream) {
    SVR_LOCK(source);
    List_remove(source->streams, List_indexOf(source->streams, stream));
    SVR_UNLOCK(source);
}

int SVRD_Source_provideData(SVRD_Source* source, void* data, size_t data_available) {
    SVRD_Stream* stream;
    IplImage* frame;

    SVR_LOCK(source);
    if(source->decoder == NULL) {
        if(source->encoding == NULL || source->frame_properties == NULL) {
            SVR_UNLOCK(source);
            return SVR_INVALIDSTATE;
        }

        source->decoder = SVR_Decoder_new(source->encoding, source->frame_properties);
    }

    SVR_Decoder_decode(source->decoder, data, data_available);
    while(SVR_Decoder_framesReady(source->decoder) > 0) {
        frame = SVR_Decoder_getFrame(source->decoder);
        for(int i = 0; (stream = List_get(source->streams, i)) != NULL; i++) {
            SVRD_Stream_inputSourceFrame(stream, frame);
        }
        SVR_Decoder_returnFrame(source->decoder, frame);
    }
    SVR_UNLOCK(source);

    return SVR_SUCCESS;
}
