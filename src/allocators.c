
#define __SVR_ALLOCATORS_C

#include <svr.h>

BlockAllocator* message_allocator = NULL;
BlockAllocator* packed_message_allocator = NULL;

StringBufferPool* message_body_buffers = NULL;
StringBufferPool* short_string_buffers = NULL;

void Allocators_init(void) {
    message_allocator = BlockAlloc_getAllocator(sizeof(SVR_Message));
    packed_message_allocator = BlockAlloc_getAllocator(sizeof(SVR_PackedMessage));
    message_body_buffers = StringBuffer_newPool(64, 4);
    short_string_buffers = StringBuffer_newPool(16, 8);
}
