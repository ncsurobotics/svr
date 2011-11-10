
#ifndef __SVR_SVR_H
#define __SVR_SVR_H

#define CV_NO_BACKWARD_COMPATIBILITY

#include <seawolf.h>

#include <arpa/inet.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <svr/forward.h>
#include <svr/cv.h>

#include <svr/lockable.h>
#include <svr/refcount.h>

#include <svr/logging.h>
#include <svr/mempool.h>
#include <svr/blockalloc.h>
#include <svr/pack.h>
#include <svr/stream.h>
#include <svr/source.h>
#include <svr/errors.h>

#include <svr/message.h>
#include <svr/net.h>
#include <svr/comm.h>

#include <svr/messagerouting.h>
#include <svr/messagehandlers.h>

#include <svr/encoding.h>
#include <svr/frameproperties.h>
#include <svr/responseset.h>

#define SVR_CRASH(m) { \
    fprintf(stderr, "[SVR_CRASH in %s] %s\n", __func__, (m)); \
    exit(-1); \
}

void SVR_init(void);
void SVR_initCore(void);
void SVR_setServerAddress(char* address);

#endif // #ifndef __SVR_SVR_H
