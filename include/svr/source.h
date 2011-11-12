
#ifndef __SVR_SOURCE_H
#define __SVR_SOURCE_H

#include <svr/forward.h>

struct SVR_Source_s {
    char* name;
    SVR_Encoding* encoding;
    Dictionary* encoding_options;
    SVR_Encoder* encoder;
    SVR_FrameProperties* frame_properties;
    void* payload_buffer;
    size_t payload_buffer_size;
};

SVR_Source* SVR_Source_new(const char* name);
int SVR_Source_destroy(SVR_Source* source);
int SVR_Source_setEncoding(SVR_Source* source, const char* encoding_name);
int SVR_Source_setFrameProperties(SVR_Source* source, SVR_FrameProperties* frame_properties);
int SVR_Source_sendFrame(SVR_Source* source, IplImage* frame);
int SVR_openServerSource(const char* name, const char* descriptor);
int SVR_closeServerSource(const char* name);
List* SVR_getSourcesList(void);
void SVR_freeSourcesList(List* sources_list);

#endif // #ifndef __SVR_SOURCE_H
