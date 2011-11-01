
#include <svr.h>

SVR_FrameProperties* SVR_FrameProperties_new(void) {
    SVR_FrameProperties* properties = malloc(sizeof(SVR_FrameProperties));

    properties->height = 0;
    properties->width = 0;
    properties->depth = 8;
    properties->channels = 3;

    return properties;
}

