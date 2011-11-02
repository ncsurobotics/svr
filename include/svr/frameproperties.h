
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
SVR_FrameProperties* SVR_FrameProperties_clone(SVR_FrameProperties* orignal_properties);
IplImage* SVR_FrameProperties_imageFromProperties(SVR_FrameProperties* properties);

#endif // #ifndef __SVR_FRAMEPROPERTIES_H
