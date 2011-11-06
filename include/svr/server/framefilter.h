
#ifndef __SVR_SERVER_FRAMEFILTER_H
#define __SVR_SERVER_FRAMEFILTER_H

#include "svr.h"
#include "svr/server/svr.h"

struct SVRs_FrameFilter_s {
    SVR_FrameProperties* input_properties;
    SVR_FrameProperties* output_properties;
    IplImage* output_frame;

    int (*filter_function)(IplImage*, IplImage*);
};

SVRs_FrameFilter* SVRs_FrameFilter_new(SVR_FrameProperties* input_properties, SVR_FrameProperties* output_properties,
        int (*filter_function)(IplImage*, IplImage*));
void SVRs_FrameFilter_destroy(SVRs_FrameFilter* frame_filter);
IplImage* SVRs_FrameFilter_apply(SVRs_FrameFilter* frame_filter, IplImage* input);

#endif // #ifndef __SVR_SERVER_FRAMEFILTER_H
