
#ifndef __SVR_SERVER_STREAM_H
#define __SVR_SERVER_STREAM_H

#include <svr/forward.h>
#include <svr/server/forward.h>

struct SVRs_Stream_s {
    char* name;

    SVRs_Client* client;
    SVRs_Source* source;

    SVR_Encoding* encoding;
    SVR_Encoder* encoder;
    SVR_FrameProperties* frame_properties;

    SVR_Stream_State state;

    void* payload_buffer;
    size_t payload_buffer_size;

    IplImage* temp_frame[2];

    SVR_LOCKABLE;
};

SVRs_Stream* SVRs_Stream_new(const char* name);
void SVRs_Stream_destroy(SVRs_Stream* stream);
void SVRs_Stream_setClient(SVRs_Stream* stream, SVRs_Client* client);
int SVRs_Stream_attachSource(SVRs_Stream* stream, SVRs_Source* source);
int SVRs_Stream_detachSource(SVRs_Stream* stream);
int SVRs_Stream_setEncoding(SVRs_Stream* stream, SVR_Encoding* encoding);
int SVRs_Stream_setChannels(SVRs_Stream* stream, int channels);
int SVRs_Stream_resize(SVRs_Stream* stream, int width, int height);

void SVRs_Stream_pause(SVRs_Stream* stream);
void SVRs_Stream_unpause(SVRs_Stream* stream);
void SVRs_Stream_inputSourceFrame(SVRs_Stream* stream, IplImage* frame);

#endif // #ifndef __SVR_SERVER_STREAM_H
