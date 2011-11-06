
#include "svr.h"
#include "svr/server/svr.h"

SVRs_FrameFilter* SVRs_FrameFilter_new(SVR_FrameProperties* input_properties, SVR_FrameProperties* output_properties, int (*filter_function)(IplImage*, IplImage*)) {
    SVRs_FrameFilter* frame_filter = malloc(sizeof(SVRs_FrameFilter));

    frame_filter->input_properties = SVR_FrameProperties_clone(input_properties);
    frame_filter->output_properties = SVR_FrameProperties_clone(output_properties);
    frame_filter->filter_function = filter_function;
    frame_filter->output_frame = SVR_FrameProperties_imageFromProperties(output_properties);

    return frame_filter;
}

void SVRs_FrameFilter_destroy(SVRs_FrameFilter* frame_filter) {
    SVR_FrameProperties_destroy(frame_filter->input_properties);
    SVR_FrameProperties_destroy(frame_filter->output_properties);
    cvReleaseImage(&frame_filter->output_frame);
    free(frame_filter);
}

IplImage* SVRs_FrameFilter_apply(SVRs_FrameFilter* frame_filter, IplImage* input) {
    frame_filter->filter_function(input, frame_filter->output_frame);
    return frame_filter->output_frame;
}
