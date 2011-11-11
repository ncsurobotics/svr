
#include "svr.h"
#include "svr/server/svr.h"

#include <highgui.h>

static SVRs_Source* FileSource_open(const char* name, Dictionary* arguments);

SVRs_SourceType SVR_SOURCE(file) = {
        .name = "file",
        .open = FileSource_open,
};

typedef struct {
    CvCapture* capture;
    pthread_t thread;
    int rate;
    bool close;
} SVRs_FileSource;

static void* FileSource_background(void* _source);
static void FileSource_close(SVRs_Source* source);

static SVRs_Source* FileSource_open(const char* name, Dictionary* arguments) {
    SVRs_FileSource* source_data;
    SVR_FrameProperties* frame_properties;
    SVRs_Source* source;
    IplImage* frame;
    char* filename;

    if(Dictionary_exists(arguments, "path") == false) {
        SVR_log(SVR_ERROR, "File sources require path argument");
        return NULL;
    }

    filename = Dictionary_get(arguments, "path");
    source_data = malloc(sizeof(SVRs_FileSource));
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

    source = SVRs_Source_new(name);
    SVRs_Source_setEncoding(source, SVR_Encoding_getByName("raw"));
    SVRs_Source_setFrameProperties(source, frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    if(source == NULL) {
        SVR_log(SVR_ERROR, Util_format("Error creating source '%s'", name));
        free(source_data);
        return NULL;
    }

    source->cleanup = FileSource_close;
    source->private_data = source_data;

    pthread_create(&source_data->thread, NULL, FileSource_background, source);
    return source;
}

static void* FileSource_background(void* _source) {
    SVRs_Source* source = (SVRs_Source*) _source;
    SVRs_FileSource* source_data = (SVRs_FileSource*) source->private_data;
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
            SVRs_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
            Util_usleep(sleep);
        }
    }

    cvReleaseCapture(&source_data->capture);

    return NULL;
}

static void FileSource_close(SVRs_Source* source) {
    SVRs_FileSource* source_data = (SVRs_FileSource*) source->private_data;

    source_data->close = true;
    pthread_join(source_data->thread, NULL);
    free(source_data);
}
