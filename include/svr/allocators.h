
#ifndef __SVR_ALLOCATORS_H
#define __SVR_ALLOCATORS_H

#include <svr/blockalloc.h>
#include <svr/streambuffer.h>

#ifndef __SVR_ALLOCATORS_C

extern BlockAllocator* message_allocator;
extern BlockAllocator* packed_message_allocator;

extern StreamBufferPool* message_body_buffers;
extern StreamBufferPool* short_string_buffers;

#endif // #ifndef __SVR_ALLOCATORS_C

void Allocators_init(void);

#endif // #ifndef __SVR_ALLOCATORS_H

