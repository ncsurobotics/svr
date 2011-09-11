
#ifndef __SVR_PACK_H
#define __SVR_PACK_H

size_t SVR_pack(void* buffer, size_t pack_offset, const char* format, ...);
size_t SVR_unpack(void* buffer, size_t pack_offset, const char* format, ...);

#endif // #ifndef __SVR_PACK_H
