
#ifndef __SVR_STRINGBUFFER_H
#define __SVR_STRINGBUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include <svr/forward.h>

struct StringBufferPool_s {
    struct StringBuffer_s* free_buffers;
    pthread_mutex_t lock;
    uint32_t buffer_size;
    uint32_t allocation_unit;
};

struct StringBuffer_s {
    uint8_t* buffer;
    uint32_t size;
    uint32_t write_index;
    uint32_t read_index;
    struct StringBuffer_s* next_free;
};

StreamBufferPool* StringBuffer_newPool(uint32_t buffer_size, uint32_t allocation_unit);
void StringBuffer_setDefaultPool(StreamBufferPool* pool);
StreamBuffer* StringBuffer_new(StreamBufferPool* pool);
void StringBuffer_free(StreamBufferPool* pool, StreamBuffer* buffer);
size_t StringBuffer_write(StreamBuffer* buffer, const char* format, ...);
size_t StringBuffer_printf(StreamBuffer* buffer, const char* format, ...);
size_t StringBuffer_getLength(StreamBuffer* buffer);
void* StringBuffer_getBuffer(StreamBuffer* buffer);
void StringBuffer_read(StreamBuffer* buffer, const char* format, ...);

#endif // #ifndef __SVR_STRINGBUFFER_H
