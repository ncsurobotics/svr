
#include <svr.h>
#include <getopt.h>

#include <highgui.h>

static void svrwatch_usage(const char* argv0);

static void svrwatch_usage(const char* argv0) {
    printf("Usage: %s [-hd] [-s ADDRESS] SOURCE_NAME...\n"
           "Seawolf Video Router Stream Watcher\n"
           "\n"
           "  -h, --help                            Show this help message\n"
           "  -d, --debug                           Enable debugging\n"
           "  -s, --server=ADDRESS                  Address of SVR server\n"
           "  -q, --quality=VALUE                   JPEG stream quality\n", argv0);
}

static SVR_Stream* svrwatch_open_stream(const char* source_name, int quality) {
    SVR_Stream* stream;

    stream = SVR_Stream_new(source_name);
    if(stream == NULL) {
        fprintf(stderr, "Could not open stream for '%s'\n", source_name);
        exit(-1);
    }
    
    if(SVR_Stream_setEncoding(stream, Util_format("jpeg:quality=%d", quality))) {
        fprintf(stderr, "Error setting encoding\n");
        exit(-1);
    }
    
    if(SVR_Stream_unpause(stream)) {
        fprintf(stderr, "Error unpausing stream\n");
        exit(-1);
    }

    return stream;
}

int main(int argc, char** argv) {
    List* sources;
    IplImage* frame;
    SVR_Stream** streams;
    int stream_count;
    int opt, indexptr;
    char* source_name;
    bool watch_all = false;
    int quality = 70;

    struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"server", 1, NULL, 's'},
        {"quality", 1, NULL, 'q'},
        {"all", 0, NULL, 'a'},
        {NULL, 0, NULL, 0}
    };

    SVR_Logging_setThreshold(SVR_LOGGING_OFF);

    while((opt = getopt_long(argc, argv, ":hds:aq:", long_options, &indexptr)) != -1) {
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

        case 'a':
            watch_all = true;
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
    
    if(watch_all == false && optind >= argc) {
        fprintf(stderr, "No source name given and --all not specified\n");
        svrwatch_usage(argv[0]);
        return -1;
    }

    if(SVR_init()) {
        fprintf(stderr, "Could not connect to SVR server!\n");
        return -1;
    }

    if(watch_all) {
        sources = SVR_getSourcesList();
        stream_count = List_getSize(sources);
        streams = malloc(sizeof(SVR_Stream*) * stream_count);

        for(int i = 0; i < stream_count; i++) {
            source_name = List_get(sources, i);
            source_name = source_name + 2;
            streams[i] = svrwatch_open_stream(source_name, quality);
        }

        SVR_freeSourcesList(sources);
    } else {
        stream_count = argc - optind;
        streams = malloc(sizeof(SVR_Stream*) * stream_count);
        for(int i = 0; i < stream_count; i++) {
            source_name = argv[optind + i];
            streams[i] = svrwatch_open_stream(source_name, quality);
        }
    }

    printf("Press 'q' to quit.\n");

    while(cvWaitKey(10) != 'q' && stream_count > 0) {
        SVR_Stream_sync();

        for(int i = 0; i < stream_count; i++) {
            frame = SVR_Stream_getFrame(streams[i], false);

            if(frame) {
                cvShowImage(Util_format("%s:%s", streams[i]->source_name, streams[i]->stream_name), frame);
                SVR_Stream_returnFrame(streams[i], frame);
            } else if(SVR_Stream_isOrphaned(streams[i])) {
                fprintf(stderr, "Stream \"%s\" orphaned\n", streams[i]->source_name);
                SVR_Stream_destroy(streams[i]);
                streams[i] = streams[stream_count - 1];
                stream_count--;
                i--;
            }
        }
    }
    
    if(stream_count == 0) {
        fprintf(stderr, "All streams orphaned\n");
        return -1;
    }

    return 0;
}
