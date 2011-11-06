
#ifndef __SVR_SERVER_FRAMEFILTERS_H
#define __SVR_SERVER_FRAMEFILTERS_H

#include <svr.h>
#include <svr/server/svr.h>

SVRs_FrameFilter* SVRs_ResizeFilter_new(SVR_FrameProperties* input_properties, int output_width, int output_height);

#endif // #ifndef __SVR_SERVER_FRAMEFILTERS_H
