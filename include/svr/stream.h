
#ifndef __SVR_STREAM_H
#define __SVR_STREAM_H

#include <svr/forward.h>

struct SVR_Stream_s {
    int __i;
};

typedef enum {
    SVR_STREAM_PAUSED,
    SVR_STREAM_RUNNING
} SVR_Stream_State;

#endif // #ifndef __SVR_STREAM_H
