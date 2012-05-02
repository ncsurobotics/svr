
#include <svr.h>
#include <svrd.h>

#include <ctype.h>

#include "sources/sources.h"

static void SVRD_Source_addType(SVRD_SourceType* source_type);
static void SVRD_Source_releaseSourceFrame(void* _source_frame);
static void SVRD_Source_cleanup(void* _source);

static Dictionary* sources = NULL;
static Dictionary* source_types = NULL;
static pthread_mutex_t sources_lock = PTHREAD_MUTEX_INITIALIZER;
static SVR_BlockAllocator* source_frame_alloc = NULL;

void SVRD_Source_init(void) {
    sources = Dictionary_new();
    source_types = Dictionary_new();
    source_frame_alloc = SVR_BlockAlloc_newAllocator(sizeof(SVRD_SourceFrame), 4);

    SVRD_Source_addType(&SVR_SOURCE(test));
    SVRD_Source_addType(&SVR_SOURCE(cam));
    SVRD_Source_addType(&SVR_SOURCE(file));
    SVRD_Source_addType(&SVR_SOURCE(v4l));
}

bool SVRD_Source_exists(const char* source_name) {
    return Dictionary_exists(sources, source_name);
}

SVRD_Source* SVRD_Source_getByName(const char* source_name) {
    SVRD_Source* source;

    pthread_mutex_lock(&sources_lock);
    source = Dictionary_get(sources, source_name);
    if(source != NULL) {
        SVR_REF(source);
    }

    pthread_mutex_unlock(&sources_lock);
    return source;
}

SVRD_Source* SVRD_Source_getLockedSource(const char* source_name) {
    SVRD_Source* source;

    pthread_mutex_lock(&sources_lock);
    source = Dictionary_get(sources, source_name);
    if(source != NULL) {
        SVR_LOCK(source);
    }

    pthread_mutex_unlock(&sources_lock);
    return source;
}

List* SVRD_Source_getSourcesList(void) {
    List* sources_list;
    List* sources_list_copy = List_new();
    char* source_name;

    pthread_mutex_lock(&sources_lock);
    sources_list = Dictionary_getKeys(sources);
    for(int i = 0; (source_name = List_get(sources_list, i)) != NULL; i++) {
        List_append(sources_list_copy, strdup(source_name));
    }
    pthread_mutex_unlock(&sources_lock);

    List_destroy(sources_list);
    return sources_list_copy;
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
    source->frame_properties = NULL;
    source->encoding = NULL;
    source->decoder = NULL;
    source->type = NULL;
    source->private_data = NULL;
    source->current_frame = NULL;
    source->closed = false;

    pthread_mutex_init(&source->current_frame_lock, NULL);
    pthread_cond_init(&source->new_frame, NULL);
    SVR_LOCKABLE_INIT(source);
    SVR_REFCOUNTED_INIT(source, SVRD_Source_cleanup);

    Dictionary_set(sources, name, source);
    pthread_mutex_unlock(&sources_lock);

    return source;
}

static void SVRD_Source_cleanup(void* _source) {
    SVRD_Source* source = (SVRD_Source*) _source;

    if(source->frame_properties) {
        SVR_FrameProperties_destroy(source->frame_properties);
    }

    if(source->decoder) {
        SVR_Decoder_destroy(source->decoder);
    }

    free(source->name);
    free(source);
}

void SVRD_Source_destroy(SVRD_Source* source) {

    SVR_LOCK(source);
    if(source->closed) {
        /* Already closed */
        SVR_UNLOCK(source);
        return;
    }

    source->closed = true;
    SVR_UNLOCK(source);

    /* Remove source from sources list and ensure source is only destroyed
       once */
    pthread_mutex_lock(&sources_lock);
    if(Dictionary_exists(sources, source->name) == false) {
        pthread_mutex_unlock(&sources_lock);
        return;
    }

    Dictionary_remove(sources, source->name);
    pthread_mutex_unlock(&sources_lock);

    /* Start shutdown of any provider if this is not a client source */
    if(source->type && source->type->close) {
        source->type->close(source);
    }

    /* Wake up any SVRD_Source_getFrame calls */
    pthread_cond_broadcast(&source->new_frame);
    
    /* Remove self reference. Object will be garbage collected once all
       references a released */
    SVR_UNREF(source);
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

SVRD_SourceFrame* SVRD_Source_getFrame(SVRD_Source* source, SVRD_Stream* stream, SVRD_SourceFrame* last_frame) {
    SVRD_SourceFrame* new_frame = NULL;

    /* Dereference the last frame */
    if(last_frame) {
        SVR_UNREF(last_frame);
    }

    /* Wait for a different frame */
    pthread_mutex_lock(&source->current_frame_lock);
    while(source->closed == false && source->current_frame == last_frame && 
          (stream == NULL || stream->state == SVR_UNPAUSED)) {
        pthread_cond_wait(&source->new_frame, &source->current_frame_lock);
    }
    
    if(source->closed == false && stream->state == SVR_UNPAUSED) {
        new_frame = source->current_frame;
        SVR_REF(new_frame);
    }

    pthread_mutex_unlock(&source->current_frame_lock);

    return new_frame;
}

void SVRD_Source_dismissPausedStreams(SVRD_Source* source) {
    pthread_cond_broadcast(&source->new_frame);
}

static void SVRD_Source_releaseSourceFrame(void* _source_frame) {
    SVRD_SourceFrame* source_frame = (SVRD_SourceFrame*) _source_frame;

    SVR_Decoder_returnFrame(source_frame->source->decoder, source_frame->frame);
    SVR_BlockAlloc_free(source_frame_alloc, source_frame);
}

int SVRD_Source_provideData(SVRD_Source* source, void* data, size_t data_available) {
    SVRD_SourceFrame* source_frame;
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

        pthread_mutex_lock(&source->current_frame_lock);
        source_frame = SVR_BlockAlloc_alloc(source_frame_alloc);
        source_frame->source = source;
        source_frame->frame = frame;
        SVR_REFCOUNTED_INIT(source_frame, SVRD_Source_releaseSourceFrame);

        if(source->current_frame) {
            SVR_UNREF(source->current_frame);
        }
        source->current_frame = source_frame;
        pthread_cond_broadcast(&source->new_frame);
        pthread_mutex_unlock(&source->current_frame_lock);
    }
    SVR_UNLOCK(source);

    return SVR_SUCCESS;
}
