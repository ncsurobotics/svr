
#include "svr.h"
#include "svrd.h"

#include <highgui.h>

static SVRD_Source* FileSource_open(const char* name, Dictionary* arguments);
static void FileSource_close(SVRD_Source* source);

SVRD_SourceType SVR_SOURCE(file) = {
        .name = "file",
        .open = FileSource_open,
        .close = FileSource_close
};

typedef struct {
    CvCapture* capture;
    pthread_t thread;
    int rate;
    bool close;
} SVRD_FileSource;

static void* FileSource_background(void* _source);

static SVRD_Source* FileSource_open(const char* name, Dictionary* arguments) {
    SVRD_FileSource* source_data;
    SVR_FrameProperties* frame_properties;
    SVRD_Source* source;
    IplImage* frame;
    char* filename;

    if(Dictionary_exists(arguments, "path") == false) {
        SVR_log(SVR_ERROR, "File sources require path argument");
        return NULL;
    }

    filename = Dictionary_get(arguments, "path");
    source_data = malloc(sizeof(SVRD_FileSource));
    source_data->capture = cvCaptureFromFile(filename);
    source_data->rate = 15;
    source_data->close = false;

    if(Dictionary_exists(arguments, "rate")) {
        source_data->rate = atoi(Dictionary_get(arguments, "rate"));
    }

    if(source_data->capture == NULL) {
        SVR_log(SVR_ERROR, Util_format("Could not open capture with file %s", filename));
        free(source_data);
        return NULL;
    }

    frame = cvQueryFrame(source_data->capture);
    if(frame == NULL) {
        SVR_log(SVR_ERROR, Util_format("Could not query frame from capture with file %s", filename));
        free(source_data);
        return NULL;
    }

    frame_properties = SVR_FrameProperties_new();
    frame_properties->width = frame->width;
    frame_properties->height = frame->height;
    frame_properties->channels = 3;
    frame_properties->depth = 8;

    source = SVRD_Source_new(name);
    SVRD_Source_setEncoding(source, "raw");
    SVRD_Source_setFrameProperties(source, frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    if(source == NULL) {
        SVR_log(SVR_ERROR, Util_format("Error creating source '%s'", name));
        free(source_data);
        return NULL;
    }

    source->private_data = source_data;

    pthread_create(&source_data->thread, NULL, FileSource_background, source);
    return source;
}

static void* FileSource_background(void* _source) {
    SVRD_Source* source = (SVRD_Source*) _source;
    SVRD_FileSource* source_data = (SVRD_FileSource*) source->private_data;
    IplImage* frame;
    float sleep = 0;

    if(source_data->rate > 0) {
        sleep = 1.0 / source_data->rate;
    }

    while(source_data->close == false) {
        frame = cvQueryFrame(source_data->capture);
        if(frame == NULL) {
            /* Reset to beginning */
            cvSetCaptureProperty(source_data->capture, CV_CAP_PROP_POS_AVI_RATIO, 0.0);
        } else {
            SVRD_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
            Util_usleep(sleep);
        }
    }

    cvReleaseCapture(&source_data->capture);

    return NULL;
}

static void FileSource_close(SVRD_Source* source) {
    SVRD_FileSource* source_data = (SVRD_FileSource*) source->private_data;

    source_data->close = true;
    pthread_join(source_data->thread, NULL);
    free(source_data);
}
