
#include <svr.h>

SVR_FrameProperties* SVR_FrameProperties_new(void) {
    SVR_FrameProperties* properties = malloc(sizeof(SVR_FrameProperties));

    properties->height = 0;
    properties->width = 0;
    properties->depth = 8;
    properties->channels = 3;

    return properties;
}

void SVR_FrameProperties_destroy(SVR_FrameProperties* properties) {
    free(properties);
}

SVR_FrameProperties* SVR_FrameProperties_clone(SVR_FrameProperties* original_properties) {
    SVR_FrameProperties* cloned_properties = malloc(sizeof(SVR_FrameProperties));
    memcpy(cloned_properties, original_properties, sizeof(SVR_FrameProperties));
    return cloned_properties;
}

IplImage* SVR_FrameProperties_imageFromProperties(SVR_FrameProperties* properties) {
    return cvCreateImage(cvSize(properties->width,
                                properties->height),
                         properties->depth,
                         properties->channels);
}
