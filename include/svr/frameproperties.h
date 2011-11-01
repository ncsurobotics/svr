
#ifndef __SVR_FRAMEPROPERTIES_H
#define __SVR_FRAMEPROPERTIES_H

#include <stdint.h>

struct SVR_FrameProperties_s {
    uint16_t height;
    uint16_t width;
    uint8_t depth;
    uint8_t channels;
};

SVR_FrameProperties* SVR_FrameProperties_new(void);

#endif // #ifndef __SVR_FRAMEPROPERTIES_H
