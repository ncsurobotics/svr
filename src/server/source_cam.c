
#include "svr.h"
#include "svr/server/svr.h"

#include <highgui.h>

static void* CamSource_background(void* _source);

static pthread_t cam_source_thread;

SVRs_Source* CamSource_open(void) {
    pthread_create(&cam_source_thread, NULL, CamSource_background, NULL);
    return NULL;
}

static void* CamSource_background(void* __unused) {
    SVR_FrameProperties* frame_properties = SVR_FrameProperties_new();
    CvCapture* capture = cvCaptureFromCAM(-1);
    SVRs_Source* source;
    IplImage* frame;

    frame = cvQueryFrame(capture);

    frame_properties->width = frame->width;
    frame_properties->height = frame->height;
    frame_properties->channels = 3;
    frame_properties->depth = 8;

    source = SVRs_Source_open(SVR_Encoding_getByName("raw"), frame_properties, "cam0");

    while(true) {
        frame = cvQueryFrame(capture);
        SVRs_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
    }

    return NULL;
}
