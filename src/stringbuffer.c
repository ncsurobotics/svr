
#include <svr.h>

static StringBufferPool* default_pool = NULL;

StringBufferPool* StringBuffer_newPool(uint32_t buffer_size, uint32_t allocation_unit) {
    StringBufferPool* pool = malloc(sizeof(StringBufferPool));
    pool->free_buffers = NULL;
    pool->buffer_size = buffer_size;
    pool->allocation_unit = allocation_unit;
    pthread_mutex_init(&pool->lock, NULL);
    return pool;
}

void StringBuffer_setDefaultPool(StringBufferPool* pool) {
    default_pool = pool;
}

StringBuffer* StringBuffer_new(StringBufferPool* pool) {
    StringBuffer* buffer;

    if(pool == NULL) {
        pool = default_pool;
    }

    pthread_mutex_lock(&pool->lock);
    if(pool->free_buffers == NULL) {
        for(int i = 0; i < pool->allocation_unit; i++) {
            buffer = malloc(sizeof(StringBuffer));
            buffer->buffer = malloc(pool->buffer_size);
            buffer->size = pool->buffer_size;
            buffer->write_index = 0;
            buffer->read_index = 0;
            buffer->next_free = pool->free_buffers;
            pool->free_buffers = buffer;
        }
    }
    buffer = pool->free_buffers;
    pool->free_buffers = buffer->next_free;
    pthread_mutex_unlock(&pool->lock);

    return buffer;
}

void StringBuffer_free(StringBufferPool* pool, StringBuffer* buffer) {
    if(pool == NULL) {
        pool = default_pool;
    }

    pthread_mutex_lock(&pool->lock);
    buffer->next_free = pool->free_buffers;
    pool->free_buffers = buffer;
    pthread_mutex_unlock(&pool->lock);
}

static void StringBuffer_makeSpace(StringBuffer* buffer, size_t space) {
    if(buffer->write_index + space >= buffer->size) {
        buffer->size = buffer->write_index + space;
        buffer->buffer = realloc(buffer->buffer, buffer->size);
    }
}

size_t StringBuffer_write(StringBuffer* buffer, const char* format, ...) {
    const char* s;
    uint32_t length;
    uint32_t old_write_index = buffer->write_index;
    va_list ap;

    va_start(ap, format);
    for(int i = 0; format[i] != '\0'; i++) {
        switch(format[i]) {
        case 'b':
            StringBuffer_makeSpace(buffer, 1);
            *((uint8_t*)(buffer->buffer + buffer->write_index)) = (uint8_t) va_arg(ap, int);
            buffer->write_index += 1;
            break;

        case 'h':
            StringBuffer_makeSpace(buffer, 2);
            *((uint16_t*)(buffer->buffer + buffer->write_index)) = (uint16_t) htons(va_arg(ap, int));
            buffer->write_index += 2;
            break;

        case 'i':
            StringBuffer_makeSpace(buffer, 4);
            *((uint32_t*)(buffer->buffer + buffer->write_index)) = (uint32_t) htonl(va_arg(ap, int));
            buffer->write_index += 2;
            break;

        case 's':
            s = va_arg(ap, char*);
            while(*s) {
                if(buffer->write_index >= buffer->size) {
                    StringBuffer_makeSpace(buffer, 16);
                }
                buffer->buffer[buffer->write_index++] = *s;
                s++;
            }
            break;

        case 'D':
            length = va_arg(ap, int);
            s = va_arg(ap, char*);
            StringBuffer_makeSpace(buffer, length);
            memcpy(buffer->buffer, s, length);
            buffer->write_index += length;
            break;
        }
    }

    va_end(ap);

    return buffer->write_index - old_write_index;
}

size_t StringBuffer_printf(StringBuffer* buffer, const char* format, ...) {
    va_list ap;
    uint32_t n;

    va_start(ap, format);

    n = vsnprintf((char*) (buffer->buffer + buffer->write_index), buffer->size - buffer->write_index, format, ap);
    if(n >= (buffer->size - buffer->write_index)) {
        /* There wasn't enough room so expand the buffer and then do it again */
        StringBuffer_makeSpace(buffer, n);
        vsnprintf((char*) (buffer->buffer + buffer->write_index), buffer->size - buffer->write_index, format, ap);
    }

    buffer->write_index += n;
    return n;
}

size_t StringBuffer_getLength(StringBuffer* buffer) {
    return buffer->write_index;
}

void* StringBuffer_getBuffer(StringBuffer* buffer) {
    return buffer->buffer;
}

void StringBuffer_read(StringBuffer* buffer, const char* format, ...) {
    va_list ap;

    va_start(ap, format);
    for(int i = 0; format[i] != '\0'; i++) {
        switch(format[i]) {
        case 'b':
            *va_arg(ap, uint8_t*) = *((uint8_t*)(buffer->buffer + buffer->read_index));
            buffer->read_index += 1;
            break;

        case 'h':
            *va_arg(ap, uint16_t*) = ntohs(*((uint16_t*)(buffer->buffer + buffer->read_index)));
            buffer->read_index += 2;
            break;

        case 'i':
            *va_arg(ap, uint32_t*) = ntohl(*((uint32_t*)(buffer->buffer + buffer->read_index)));
            buffer->read_index += 4;
            break;

        case 's':
            *va_arg(ap, char**) = (char*) (buffer->buffer + buffer->read_index);
            while(buffer->buffer[buffer->read_index]) {
                buffer->read_index += 1;
            }
            break;
        }
    }
}
