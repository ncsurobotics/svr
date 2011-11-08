
#ifndef __SVR_SERVER_SOURCE_H
#define __SVR_SERVER_SOURCE_H

#include <svr/forward.h>

struct SVRs_Source_s {
    SVR_Decoder* decoder;
    SVR_FrameProperties* frame_properties;
    List* streams;
    char* name;

    void (*cleanup)(SVRs_Source*);
    void* private_data;

    SVR_LOCKABLE;
};

struct SVRs_SourceType_s {
    const char* name;
    SVRs_Source* (*open)(const char* name, Dictionary* arguments);
};

#define SVR_SOURCE(name) __svr_##name##_source

/**
 * Give encoded data to the source to pass to its registered streams
 */
void SVRs_Source_provideData(SVRs_Source* source, void* data, size_t data_available);

void SVRs_Source_init(void);
SVRs_Source* SVRs_Source_new(const char* name, SVR_Encoding* encoding, SVR_FrameProperties* frame_properties);
SVRs_Source* SVRs_Source_openInstance(const char* source_name, const char* descriptor_orig);
void SVRs_Source_fromFile(const char* filename);
SVRs_Source* SVRs_getSourceByName(const char* source_name);
SVR_FrameProperties* SVRs_Source_getFrameProperties(SVRs_Source* source);

void SVRs_Source_registerStream(SVRs_Source* source, SVRs_Stream* stream);
void SVRs_Source_unregisterStream(SVRs_Source* source, SVRs_Stream* stream);

#endif // #ifndef __SVR_SERVER_SOURCE_H

