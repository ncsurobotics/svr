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
 *
 * These functions are inspired by the Python struct module. They provide a way
 * for a data to be packed into a byte buffer and later unpacked from a byte
 * buffer in a way that is safe to use regardless of byte order or structure
 * padding.
 */

/**
 * \brief Pack a buffer
 *
 * Pack a number of values into the buffer at the given offset based on the
 * format string given. The format string specifies the types of the values to
 * be packed as a string of format characters. Valid characters are b, h, i, s,
 * and D. The characters b, h, and i represent unsigned 8 bit, 16 bit, and 32
 * bit integers types respectively. An s specifies a NULL-terminated string. A D
 * is a fixed length byte array. The length is given immediately before the
 * pointer in the argument string.
 *
 * \param buffer Output buffer
 * \param pack_offset Offset into the buffer to write
 * \param format Format string as described above
 * \param ... Values to pack
 * \return New pack offset
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

/**
 * \brief Unpack a buffer
 *
 * Unpack the values described by the format string.
 *
 * \param buffer Buffer to unpack from
 * \param pack_offset Offset in the buffer to unpack from
 * \param format Format string as described above
 * \param ... Pointers to variables to store the unpacked values in
 * \return New pack offset
 */
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
