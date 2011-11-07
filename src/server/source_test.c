
#include "svr.h"
#include "svr/server/svr.h"

#define WIDTH 320
#define HEIGHT 240

static void* TestSource_background(void* _source);

static pthread_t test_source_thread;

SVRs_Source* TestSource_open(void) {
    pthread_create(&test_source_thread, NULL, TestSource_background, NULL);
    return NULL;
}

static void* TestSource_background(void* __unused) {
    SVR_FrameProperties* frame_properties = SVR_FrameProperties_new();
    SVRs_Source* source;
    IplImage* frame;
    int block_x = 0, block_y = 0;
    CvScalar colors[] = {CV_RGB(255, 0, 0),
                         CV_RGB(0, 255, 0),
                         CV_RGB(0, 0, 255)
    };
    
    frame_properties->width = WIDTH;
    frame_properties->height = HEIGHT;
    frame_properties->channels = 3;
    frame_properties->depth = 8;

    source = SVRs_Source_open(SVR_Encoding_getByName("raw"), frame_properties, "test");
    frame = SVR_FrameProperties_imageFromProperties(frame_properties);

    while(true) {
        Util_usleep(1.0/25.0);

        /* Generate image with a single white rectangle that moves */
        cvRectangle(frame, cvPoint(0, 0), cvPoint(WIDTH - 1, HEIGHT - 1), CV_RGB(0, 0, 0), CV_FILLED, 8, 0);
        cvRectangle(frame, cvPoint(block_x, block_y), cvPoint(block_x + 64, block_y + 48), colors[rand() % 3], CV_FILLED, 8, 0);
        
        /* Move to next block */
        block_x = (block_x + 64) % WIDTH;
        if(block_x == 0) {
            block_y = (block_y + 48) % HEIGHT;
        }

        SVRs_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
    }

    return NULL;
}
