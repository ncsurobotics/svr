
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
#include <svr/mempool.h>
#include <svr/allocators.h>
#include <svr/blockalloc.h>
#include <svr/pack.h>

#include <svr/message.h>

#include <svr/client.h>
#include <svr/encoding.h>
#include <svr/frameproperties.h>
#include <svr/reencoder.h>
#include <svr/source.h>
#include <svr/stream.h>

#endif // #ifndef __SVR_SVR_H
