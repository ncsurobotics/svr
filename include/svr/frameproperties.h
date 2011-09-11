
#ifndef __SVR_FRAMEPROPERTIES_H
#define __SVR_FRAMEPROPERTIES_H

#include <stdint.h>

struct FrameProperties_s {
    uint16_t height;
    uint16_t width;
    uint8_t depth;
    uint8_t channels;
};

#endif // #ifndef __SVR_FRAMEPROPERTIES_H
