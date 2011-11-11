
#include <svr.h>
#include <getopt.h>

#include <highgui.h>

static void svrwatch_usage(const char* argv0);

static void svrwatch_usage(const char* argv0) {
    printf("Usage: %s [-hd] [-s ADDRESS] SOURCE_NAME\n"
           "Seawolf Video Router Stream Watcher\n"
           "\n"
           "  -h, --help                            Show this help message\n"
           "  -d, --debug                           Enable debugging\n"
           "  -s, --server=ADDRESS                  Address of SVR server\n"
           "  -q, --quality=VALUE                   JPEG stream quality\n", argv0);
}

int main(int argc, char** argv) {
    IplImage* frame;
    SVR_Stream* stream;
    int opt, indexptr;
    char* source_name;
    int quality = 70;

    struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"server", 1, NULL, 's'},
        {"quality", 1, NULL, 'q'},
        {NULL, 0, NULL, 0}
    };

    SVR_Logging_setThreshold(SVR_LOGGING_OFF);

    while((opt = getopt_long(argc, argv, ":hds:q:", long_options, &indexptr)) != -1) {
        switch(opt) {
        case 'h':
            svrwatch_usage(argv[0]);
            return 0;

        case 'd':
            SVR_Logging_setThreshold(SVR_DEBUG);
            break;

        case 's':
            SVR_setServerAddress(optarg);
            break;

        case 'q':
            quality = atoi(optarg);
            if(quality < 5 || quality > 100) {
                fprintf(stderr, "JPEG quality must be between 5 and 100 inclusive\n\n");
                svrwatch_usage(argv[0]);
                return -1;
            }
            break;

        case '?':
            fprintf(stderr, "Unknown switch '%s'\n\n", argv[optind - 1]);
            svrwatch_usage(argv[0]);
            return -1;

        default:
            break;
        }
    }
    
    if(optind >= argc) {
        fprintf(stderr, "No source name given\n");
        svrwatch_usage(argv[0]);
        return -1;
    }

    if(SVR_init()) {
        fprintf(stderr, "Could not connect to SVR server!\n");
        return -1;
    }

    source_name = argv[argc - 1];
    stream = SVR_Stream_new(source_name);
    if(stream == NULL) {
        fprintf(stderr, "Could not open stream for '%s'\n", source_name);
        return -1;
    }
    
    if(SVR_Stream_setEncoding(stream, Util_format("jpeg:quality=%d", quality))) {
        fprintf(stderr, "Error setting encoding\n");
        return -1;
    }

    if(SVR_Stream_unpause(stream)) {
        fprintf(stderr, "Error unpausing stream\n");
        return -1;
    }

    printf("Press 'q' to quit.\n");

    while(cvWaitKey(10) != 'q') {
        frame = SVR_Stream_getFrame(stream, true);
        cvShowImage(source_name, frame);
        SVR_Stream_returnFrame(stream, frame);
    }

    return 0;
}
