
#ifndef __SVR_STREAM_H
#define __SVR_STREAM_H

#include <svr/forward.h>
#include <svr/lockable.h>
#include <svr/cv.h>

struct SVR_Stream_s {
    const char* stream_name;
    const char* source_name;
    IplImage* current_frame;
    SVR_Decoder* decoder;

    pthread_cond_t new_frame;
    SVR_LOCKABLE;
};

typedef enum {
    SVR_PAUSED,
    SVR_UNPAUSED
} SVR_Stream_State;

void SVR_Stream_init(void);
SVR_Stream* SVR_Stream_new(const char* stream_name, const char* source);
int SVR_Stream_open(SVR_Stream* stream);
IplImage* SVR_Stream_getFrame(SVR_Stream* stream, bool wait);
void SVR_Stream_returnFrame(SVR_Stream* stream, IplImage* frame);
void SVR_Stream_provideData(const char* stream_name, void* buffer, size_t n);

#endif // #ifndef __SVR_STREAM_H
