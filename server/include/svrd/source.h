
#ifndef __SVR_SERVER_SOURCE_H
#define __SVR_SERVER_SOURCE_H

#include <svr/forward.h>

struct SVRD_Source_s {
    char* name;

    SVR_Encoding* encoding;
    SVR_Decoder* decoder;
    SVR_FrameProperties* frame_properties;

    List* streams;

    SVRD_SourceType* type;
    void* private_data;

    SVR_LOCKABLE;
};

struct SVRD_SourceType_s {
    const char* name;
    SVRD_Source* (*open)(const char* name, Dictionary* arguments);
    void (*close)(SVRD_Source* source);
};

#define SVR_SOURCE(name) __svr_##name##_source

void SVRD_Source_init(void);
SVRD_Source* SVRD_Source_getByName(const char* source_name);
SVRD_Source* SVRD_Source_openInstance(const char* source_name, const char* descriptor_orig);
void SVRD_Source_fromFile(const char* filename);

SVRD_Source* SVRD_Source_new(const char* name);
void SVRD_Source_destroy(SVRD_Source* source);
SVR_FrameProperties* SVRD_Source_getFrameProperties(SVRD_Source* source);
int SVRD_Source_setEncoding(SVRD_Source* source, SVR_Encoding* encoding);
int SVRD_Source_setFrameProperties(SVRD_Source* source, SVR_FrameProperties* frame_properties);
void SVRD_Source_registerStream(SVRD_Source* source, SVRD_Stream* stream);
void SVRD_Source_unregisterStream(SVRD_Source* source, SVRD_Stream* stream);
int SVRD_Source_provideData(SVRD_Source* source, void* data, size_t data_available);

#endif // #ifndef __SVR_SERVER_SOURCE_H
