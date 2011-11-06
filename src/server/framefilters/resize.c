
#include "framefilters.h"

static int SVRs_ResizeFilter_apply(IplImage* source, IplImage* dest) {
    cvResize(source, dest, CV_INTER_NN);
    return 0;
}

SVRs_FrameFilter* SVRs_ResizeFilter_new(SVR_FrameProperties* input_properties, int output_width, int output_height) {
    SVR_FrameProperties* output_properties = SVR_FrameProperties_clone(input_properties);
    SVRs_FrameFilter* frame_filter;

    output_properties->width = output_width;
    output_properties->height = output_height;

    frame_filter = SVRs_FrameFilter_new(input_properties, output_properties, SVRs_ResizeFilter_apply);
    SVR_FrameProperties_destroy(output_properties);

    return frame_filter;
}
