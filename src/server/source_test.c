
#include "svr.h"
#include "svr/server/svr.h"

#include <svr/cv.h>

static void* TestSource_background(void* _source);

static pthread_t test_source_thread;

SVRs_Source* TestSource_open(void) {
    SVR_FrameProperties* frame_properties = SVR_FrameProperties_new();
    SVRs_Source* source;

    frame_properties->width = 320;
    frame_properties->height = 240;
    frame_properties->channels = 1;
    frame_properties->depth = 8;

    source = SVRs_Source_new(SVR_Encoding_getByName("raw"), frame_properties);

    pthread_create(&test_source_thread, NULL, TestSource_background, source);

    return source;
}

static void* TestSource_background(void* _source) {
    SVRs_Source* source = (SVRs_Source*) _source;
    IplImage* frame = cvCreateImage(cvSize(320, 240), IPL_DEPTH_8U, 1);
    int block_x = 0, block_y = 0;
    
    while(true) {
        Util_usleep(0.250);

        /* Generate image with a single white rectangle that moves */
        cvRectangle(frame, cvPoint(0, 0), cvPoint(320 - 1, 240 - 1), CV_RGB(0, 0, 0), CV_FILLED, 8, 0);
        cvRectangle(frame, cvPoint(block_x, block_y), cvPoint(block_x + 64, block_y + 48), CV_RGB(255, 255, 255), CV_FILLED, 8, 0);
        
        /* Move to next block */
        block_x = (block_x + 64) % 320;
        if(block_x == 0) {
            block_y = (block_y + 48) % 240;
        }

        SVRs_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
    }

    return NULL;
}
