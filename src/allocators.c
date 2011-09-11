
#define __SVR_ALLOCATORS_C

#include <svr.h>

BlockAllocator* svr_alloc_message = NULL;
BlockAllocator* svr_alloc_packed_message = NULL;

StreamBufferPool* svr_alloc_message_body = NULL;
StreamBufferPool* svr_alloc_string = NULL;

void Allocators_init(void) {
    svr_alloc_message = BlockAlloc_newAllocator(sizeof(SVR_Message));
    svr_alloc_packed_message = BlockAlloc_newAllocator(sizeof(SVR_PackedMessage));
    svr_alloc_message_body = StringBuffer_newPool(64, 4);
    svr_alloc_string = StringBuffer_newPool(16, 8);
}
