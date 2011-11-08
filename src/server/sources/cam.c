
#include "svr.h"
#include "svr/server/svr.h"

#include <highgui.h>

static SVRs_Source* CamSource_open(const char* name, Dictionary* arguments);

SVRs_SourceType SVR_SOURCE(cam) = {
        .name = "cam",
        .open = CamSource_open,
};

typedef struct {
    CvCapture* capture;
    pthread_t thread;
    bool close;
} SVRs_CamSource;

static void* CamSource_background(void* _source);
static void CamSource_close(SVRs_Source* source);

static SVRs_Source* CamSource_open(const char* name, Dictionary* arguments) {
    SVRs_CamSource* source_data = malloc(sizeof(SVRs_CamSource));
    SVR_FrameProperties* frame_properties;
    SVRs_Source* source;
    IplImage* frame;
    int index = -1;

    if(Dictionary_exists(arguments, "index")) {
        index = atoi(Dictionary_get(arguments, "index"));
    }

    source_data->capture = cvCaptureFromCAM(index);
    source_data->close = false;

    if(source_data->capture == NULL) {
        SVR_log(SVR_ERROR, Util_format("Could not open camera with index %d", index));
        return NULL;
    }

    frame = cvQueryFrame(source_data->capture);
    if(frame == NULL) {
        SVR_log(SVR_ERROR, Util_format("Could not query frame from device with index %d", index));
        return NULL;
    }

    frame_properties = SVR_FrameProperties_new();
    frame_properties->width = frame->width;
    frame_properties->height = frame->height;
    frame_properties->channels = 3;
    frame_properties->depth = 8;

    source = SVRs_Source_new(name, SVR_Encoding_getByName("raw"), frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    if(source == NULL) {
        SVR_log(SVR_ERROR, Util_format("Error creating source '%s'", name));
        cvReleaseCapture(&source_data->capture);
        return NULL;
    }

    source->cleanup = CamSource_close;
    source->private_data = source_data;

    pthread_create(&source_data->thread, NULL, CamSource_background, source);
    return source;
}

static void* CamSource_background(void* _source) {
    SVRs_Source* source = (SVRs_Source*) _source;
    SVRs_CamSource* source_data = (SVRs_CamSource*) source->private_data;
    IplImage* frame;

    while(source_data->close == false) {
        frame = cvQueryFrame(source_data->capture);
        if(frame == NULL) {
            SVR_log(SVR_CRITICAL, Util_format("Error retrieving frame from camera! (%s)", source->name));
            Util_usleep(1.0);
        } else {
            SVRs_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
        }
    }

    cvReleaseCapture(&source_data->capture);

    return NULL;
}

static void CamSource_close(SVRs_Source* source) {
    SVRs_CamSource* source_data = (SVRs_CamSource*) source->private_data;

    source_data->close = true;
    pthread_join(source_data->thread, NULL);
    free(source_data);
}
