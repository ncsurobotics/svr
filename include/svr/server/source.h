
#ifndef __SVR_SERVER_SOURCE_H
#define __SVR_SERVER_SOURCE_H

#include <svr/forward.h>

struct SVRs_Source_s {
    char* name;

    SVR_Encoding* encoding;
    SVR_Decoder* decoder;
    SVR_FrameProperties* frame_properties;

    List* streams;

    void (*cleanup)(SVRs_Source*);
    void* private_data;

    SVR_LOCKABLE;
};

struct SVRs_SourceType_s {
    const char* name;
    SVRs_Source* (*open)(const char* name, Dictionary* arguments);
};

#define SVR_SOURCE(name) __svr_##name##_source

void SVRs_Source_init(void);
SVRs_Source* SVRs_Source_getByName(const char* source_name);
SVRs_Source* SVRs_Source_openInstance(const char* source_name, const char* descriptor_orig);
void SVRs_Source_fromFile(const char* filename);

SVRs_Source* SVRs_Source_new(const char* name);

SVR_FrameProperties* SVRs_Source_getFrameProperties(SVRs_Source* source);
int SVRs_Source_setEncoding(SVRs_Source* source, SVR_Encoding* encoding);
int SVRs_Source_setFrameProperties(SVRs_Source* source, SVR_FrameProperties* frame_properties);
void SVRs_Source_registerStream(SVRs_Source* source, SVRs_Stream* stream);
void SVRs_Source_unregisterStream(SVRs_Source* source, SVRs_Stream* stream);
int SVRs_Source_provideData(SVRs_Source* source, void* data, size_t data_available);

#endif // #ifndef __SVR_SERVER_SOURCE_H
