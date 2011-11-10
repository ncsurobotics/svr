
#include <svr.h>
#include <highgui.h>

int main(void) {
    SVR_Source* source;
    SVR_Stream* stream1;
    SVR_Stream* stream2;

    SVR_init();

    source = SVR_Source_new("clientsource");
    stream1 = SVR_Stream_new("test");

    SVR_Stream_unpause(stream1);

    /* Prime source */
    SVR_Source_sendFrame(source, SVR_Stream_getFrame(stream1));

    stream2 = SVR_Stream_new("clientsource");

    while(cvWaitKey() != 'q') {
        
    }
    
    return 0;
}
