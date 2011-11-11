
#include "svr.h"
#include "svrd.h"

#include <highgui.h>

static SVRD_Source* CamSource_open(const char* name, Dictionary* arguments);
static void CamSource_close(SVRD_Source* source);

SVRD_SourceType SVR_SOURCE(cam) = {
        .name = "cam",
        .open = CamSource_open,
        .close = CamSource_close
};

typedef struct {
    CvCapture* capture;
    pthread_t thread;
    bool close;
} SVRD_CamSource;

static void* CamSource_background(void* _source);

static SVRD_Source* CamSource_open(const char* name, Dictionary* arguments) {
    SVRD_CamSource* source_data = malloc(sizeof(SVRD_CamSource));
    SVR_FrameProperties* frame_properties;
    SVRD_Source* source;
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

    for(int i = 0; (frame = cvQueryFrame(source_data->capture)) == NULL && i < 5; i++);
    if(frame == NULL) {
        SVR_log(SVR_ERROR, Util_format("Could not query frame from device with index %d", index));
        return NULL;
    }

    frame_properties = SVR_FrameProperties_new();
    frame_properties->width = frame->width;
    frame_properties->height = frame->height;
    frame_properties->channels = 3;
    frame_properties->depth = 8;

    source = SVRD_Source_new(name);
    SVRD_Source_setEncoding(source, SVR_Encoding_getByName("raw"));
    SVRD_Source_setFrameProperties(source, frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    if(source == NULL) {
        SVR_log(SVR_ERROR, Util_format("Error creating source '%s'", name));
        cvReleaseCapture(&source_data->capture);
        free(source_data);
        return NULL;
    }

    source->private_data = source_data;

    pthread_create(&source_data->thread, NULL, CamSource_background, source);
    return source;
}

static void* CamSource_background(void* _source) {
    SVRD_Source* source = (SVRD_Source*) _source;
    SVRD_CamSource* source_data = (SVRD_CamSource*) source->private_data;
    IplImage* frame;

    while(source_data->close == false) {
        frame = cvQueryFrame(source_data->capture);
        if(frame == NULL) {
            SVR_log(SVR_CRITICAL, Util_format("Error retrieving frame from camera! (%s)", source->name));
            Util_usleep(1.0);
        } else {
            SVRD_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
        }
    }

    cvReleaseCapture(&source_data->capture);

    return NULL;
}

static void CamSource_close(SVRD_Source* source) {
    SVRD_CamSource* source_data = (SVRD_CamSource*) source->private_data;

    source_data->close = true;
    pthread_join(source_data->thread, NULL);
    free(source_data);
}
