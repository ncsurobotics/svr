/**
 * \file
 * \brief Packing
 */

#include "svr.h"

/**
 * \defgroup Packing Packing
 * \ingroup Util
 * \brief Byte buffer packing/unpacking
 * \{
 */

size_t SVR_pack(void* buffer, size_t pack_offset, const char* format, ...) {
    uint8_t* p = (uint8_t*) buffer;

    const char* s;
    int length;

    va_list ap;

    va_start(ap, format);
    for(int i = 0; format[i] != '\0'; i++) {
        switch(format[i]) {
        case 'b':
            *((uint8_t*)(p + pack_offset)) = (uint8_t) va_arg(ap, int);
            pack_offset += 1;
            break;

        case 'h':
            *((uint16_t*)(p + pack_offset)) = (uint16_t) htons(va_arg(ap, int));
            pack_offset += 2;
            break;

        case 'i':
            *((uint32_t*)(p + pack_offset)) = (uint32_t) htonl(va_arg(ap, int));
            pack_offset += 4;
            break;

        case 's':
            s = va_arg(ap, char*) - 1;
            do {
                s++;
                p[pack_offset++] = (uint8_t) *s;
            } while(*s);
            break;

        case 'D':
            length = va_arg(ap, int);
            s = va_arg(ap, char*);
            memcpy(p + pack_offset, s, length);
            pack_offset += length;
            break;
        }
    }

    va_end(ap);

    return pack_offset;
}

size_t SVR_unpack(void* buffer, size_t pack_offset, const char* format, ...) {
    uint8_t* p = buffer;
    va_list ap;

    va_start(ap, format);
    for(int i = 0; format[i] != '\0'; i++) {
        switch(format[i]) {
        case 'b':
            *va_arg(ap, uint8_t*) = *((uint8_t*)(p + pack_offset));
            pack_offset += 1;
            break;

        case 'h':
            *va_arg(ap, uint16_t*) = ntohs(*((uint16_t*)(p + pack_offset)));
            pack_offset += 2;
            break;

        case 'i':
            *va_arg(ap, uint32_t*) = ntohl(*((uint32_t*)(p + pack_offset)));
            pack_offset += 4;
            break;

        case 's':
            *va_arg(ap, char**) = (char*) (p + pack_offset);
            do {
                pack_offset += 1;
            } while(p[pack_offset - 1] != '\0');
            break;
        }
    }

    return pack_offset;
}

/** \} */
