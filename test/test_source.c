
#include <svr.h>
#include <highgui.h>

int main(void) {
    IplImage* frame;
    SVR_Source* source;
    SVR_Stream* stream1;
    SVR_Stream* stream2;

    SVR_init();

    source = SVR_Source_new("clientsource");
    if(source == NULL) {
        fprintf(stderr, "Failed to make source\n");
    }

    stream1 = SVR_Stream_new("test");
    SVR_Source_setEncoding(source, "raw");

    SVR_Stream_resize(stream1, 200, 200);
    SVR_Stream_unpause(stream1);

    /* Prime source */
    SVR_Source_sendFrame(source, SVR_Stream_getFrame(stream1, true));

    stream2 = SVR_Stream_new("clientsource");
    SVR_Stream_unpause(stream2);

    while(cvWaitKey(10) != 'q') {
        frame = SVR_Stream_getFrame(stream1, true);
        SVR_Source_sendFrame(source, frame);
        SVR_Stream_returnFrame(stream1, frame);

        frame = SVR_Stream_getFrame(stream2, true);
        cvShowImage("test", frame);
        SVR_Stream_returnFrame(stream2, frame);
    }

    SVR_Stream_destroy(stream1);
    SVR_Stream_destroy(stream2);
    SVR_Source_destroy(source);

    source = SVR_Source_new("clientsource");
    if(source == NULL) {
        fprintf(stderr, "Failed to make source\n");
    }

    stream1 = SVR_Stream_new("test");
    SVR_Source_setEncoding(source, "raw");

    SVR_Stream_resize(stream1, 200, 200);
    SVR_Stream_unpause(stream1);

    /* Prime source */
    SVR_Source_sendFrame(source, SVR_Stream_getFrame(stream1, true));

    stream2 = SVR_Stream_new("clientsource");
    SVR_Stream_unpause(stream2);

    while(cvWaitKey(10) != 'q') {
        frame = SVR_Stream_getFrame(stream1, true);
        SVR_Source_sendFrame(source, frame);
        SVR_Stream_returnFrame(stream1, frame);

        frame = SVR_Stream_getFrame(stream2, true);
        cvShowImage("test", frame);
        SVR_Stream_returnFrame(stream2, frame);
    }

    return 0;
}
