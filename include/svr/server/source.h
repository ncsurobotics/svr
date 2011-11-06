
#ifndef __SVR_SERVER_SOURCE_H
#define __SVR_SERVER_SOURCE_H

#include <svr/forward.h>

struct SVRs_Source_s {
    SVR_Decoder* decoder;
    SVR_FrameProperties* frame_properties;
    List* streams;
    char* name;

    SVR_LOCKABLE;
};

/**
 * Give encoded data to the source to pass to its registered streams
 */
void SVRs_Source_provideData(SVRs_Source* source, void* data, size_t data_available);

void SVRs_Source_init(void);
SVRs_Source* SVRs_Source_new(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties);
SVRs_Source* SVRs_Source_open(SVR_Encoding* encoding, SVR_FrameProperties* frame_properties, const char* name);
SVRs_Source* SVRs_getSourceByName(const char* source_name);
SVR_FrameProperties* SVRs_Source_getFrameProperties(SVRs_Source* source);

void SVRs_Source_registerStream(SVRs_Source* source, SVRs_Stream* stream);
void SVRs_Source_unregisterStream(SVRs_Source* source, SVRs_Stream* stream);

SVRs_Source* TestSource_open(void);
SVRs_Source* CamSource_open(void);

#endif // #ifndef __SVR_SERVER_SOURCE_H

