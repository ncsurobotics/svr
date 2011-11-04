
#include <svr.h>
#include <highgui.h>

int main(void) {
    SVR_Stream* stream;
    IplImage* frame;

    SVR_init();
    stream = SVR_Stream_new("stream1", "test");

    if(SVR_Stream_open(stream)) {
        fprintf(stderr, "Error opening stream\n");
        return -1;
    }

    printf("Stream opened\n");

    while(cvWaitKey(10) != 'q') {
        frame = SVR_Stream_getFrame(stream, true);
        printf("Got frame\n");
        cvShowImage("stream1", frame);
        SVR_Stream_returnFrame(stream, frame);
    }

    return 0;
}
