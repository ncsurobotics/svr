
#include <svr.h>
#include <highgui.h>

int main(void) {
    SVR_Stream* stream1;
    SVR_Stream* stream2;
    SVR_Stream* stream3;
    IplImage* frame;

    SVR_init();

    stream1 = SVR_Stream_new("stream1", "cam0");
    stream2 = SVR_Stream_new("stream2", "cam0");
    stream3 = SVR_Stream_new("stream3", "cam0");

    if(stream1 == NULL) {
        fprintf(stderr, "Error opening stream1\n");
        return -1;
    }

    if(stream2 == NULL) {
        fprintf(stderr, "Error opening stream2\n");
        return -1;
    }

    if(stream3 == NULL) {
        fprintf(stderr, "Error opening stream3\n");
        return -1;
    }

    printf("Streams opened\n");

    if(SVR_Stream_resize(stream1, 640, 480)) {
        fprintf(stderr, "Error resizing stream1\n");
        return -1;
    }

    if(SVR_Stream_resize(stream2, 320, 240)) {
        fprintf(stderr, "Error resizing stream1\n");
        return -1;
    }

    if(SVR_Stream_resize(stream3, 160, 120)) {
        fprintf(stderr, "Error resizing stream2\n");
        return -1;
    }

    SVR_Stream_unpause(stream1);
    SVR_Stream_unpause(stream2);
    SVR_Stream_unpause(stream3);

    while(cvWaitKey(10) != 'q') {
        frame = SVR_Stream_getFrame(stream1, true);
        cvShowImage("stream1", frame);
        SVR_Stream_returnFrame(stream1, frame);

        frame = SVR_Stream_getFrame(stream2, true);
        cvShowImage("stream2", frame);
        SVR_Stream_returnFrame(stream2, frame);

        frame = SVR_Stream_getFrame(stream3, true);
        cvShowImage("stream3", frame);
        SVR_Stream_returnFrame(stream3, frame);
    }

    SVR_Stream_destroy(stream1);
    SVR_Stream_destroy(stream2);
    SVR_Stream_destroy(stream3);
    cvDestroyAllWindows();

    stream1 = SVR_Stream_new("stream1", "test");
    if(stream1 == NULL) {
        fprintf(stderr, "Error opening stream 1");
    }

    SVR_Stream_unpause(stream1);

    while(cvWaitKey(10) != 'q') {
        frame = SVR_Stream_getFrame(stream1, true);
        cvShowImage("stream1", frame);
        SVR_Stream_returnFrame(stream1, frame);
    }

    return 0;
}
