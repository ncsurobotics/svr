
#include <svr.h>
#include <getopt.h>

#include <highgui.h>

#define POLLING_FREQ 5

static int quality = 70;
static bool list_stale = false;
static pthread_mutex_t list_stale_lock = PTHREAD_MUTEX_INITIALIZER;

static void svrwatch_usage(const char* argv0) {
    printf("Usage: %s [-hdra] [-q QUALITY] [-s ADDRESS] SOURCE_NAME...\n"
           "Seawolf Video Router Stream Watcher\n"
           "\n"
           "  -h, --help                            Show this help message\n"
           "  -d, --debug                           Enable debugging\n"
           "  -s, --server=ADDRESS                  Address of SVR server\n"
           "  -r, --raw                             Use raw encoding (default is JPEG)\n"
           "  -q, --quality=VALUE                   JPEG stream quality\n"
           "  -a, --all                             Watch all streams\n", argv0);
}

static SVR_Stream* svrwatch_open_stream(const char* source_name) {
    SVR_Stream* stream;
    char* encoding;

    stream = SVR_Stream_new(source_name);
    if(stream == NULL) {
        fprintf(stderr, "Could not open stream for '%s'\n", source_name);
        exit(-1);
    }

    if(quality == -1) {  /* Raw encoding */
        encoding = "raw";
    } else {
        encoding = Util_format("jpeg:quality=%d", quality);
    }

    if(SVR_Stream_setEncoding(stream, encoding)) {
        fprintf(stderr, "Error setting encoding\n");
        exit(-1);
    }

    if(SVR_Stream_unpause(stream)) {
        fprintf(stderr, "Error unpausing stream\n");
        exit(-1);
    }

    return stream;
}

static List* svrwatch_streams_list(Dictionary* streams) {
    List* source_names = Dictionary_getKeys(streams);
    char* source_name;
    List* stream_list = List_new();

    for(int i = 0; (source_name = List_get(source_names, i)) != NULL; i++) {
        List_append(stream_list, Dictionary_get(streams, source_name));
    }

    List_destroy(source_names);

    return stream_list;
}

static bool svrwatch_open_new(Dictionary* streams) {
    SVR_Stream* stream;
    List* sources;
    int stream_count;
    char* source_name;
    bool new_stream = false;

    sources = SVR_getSourcesList();
    stream_count = List_getSize(sources);

    for(int i = 0; i < stream_count; i++) {
        source_name = List_get(sources, i);
        source_name = source_name + 2;

        if(Dictionary_exists(streams, source_name) == false) {
            stream = svrwatch_open_stream(source_name);
            Dictionary_set(streams, source_name, stream);
            new_stream = true;
        }
    }

    SVR_freeSourcesList(sources);
    return new_stream;
}

static void* svrwatch_polling_thread(void* _streams) {
    Dictionary* streams = (Dictionary*) _streams;

    while(true) {
        if(svrwatch_open_new(streams)) {
            pthread_mutex_lock(&list_stale_lock);
            list_stale = true;
            pthread_mutex_unlock(&list_stale_lock);
        }
        Util_usleep(1.0/POLLING_FREQ);
    }

    return NULL;
}

int main(int argc, char** argv) {
    IplImage* frame;
    Dictionary* streams;
    SVR_Stream* stream;
    List* streams_list;
    int stream_count;
    int opt, indexptr;
    char* source_name;
    bool watch_all = false;
    bool raw = false;
    pthread_t polling_thread;

    struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"server", 1, NULL, 's'},
        {"raw", 0, NULL, 'r'},
        {"quality", 1, NULL, 'q'},
        {"all", 0, NULL, 'a'},
        {NULL, 0, NULL, 0}
    };

    SVR_Logging_setThreshold(SVR_LOGGING_OFF);

    while((opt = getopt_long(argc, argv, ":hdrs:aq:", long_options, &indexptr)) != -1) {
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

        case 'r':
            raw = true;
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

    if(raw) {
        quality = -1;
    }

    streams = Dictionary_new();

    if(watch_all) {
        svrwatch_open_new(streams);
        pthread_create(&polling_thread, NULL, svrwatch_polling_thread, streams);
    } else {
        stream_count = argc - optind;
        for(int i = 0; i < stream_count; i++) {
            source_name = argv[optind + i];
            stream = svrwatch_open_stream(source_name);

            Dictionary_set(streams, source_name, stream);
        }
    }

    streams_list = svrwatch_streams_list(streams);
    stream_count = List_getSize(streams_list);

    printf("Press 'q' to quit.\n");

    while(cvWaitKey(10) != 'q' && (stream_count > 0 || watch_all)) {
        pthread_mutex_lock(&list_stale_lock);
        if(list_stale) {
            list_stale = false;
            List_destroy(streams_list);
            streams_list = svrwatch_streams_list(streams);
            stream_count = List_getSize(streams_list);
        }
        pthread_mutex_unlock(&list_stale_lock);

        for(int i = 0; i < stream_count; i++) {
            stream = List_get(streams_list, i);
            frame = SVR_Stream_getFrame(stream, false);

            if(frame) {
                cvShowImage(Util_format("%s:%s", stream->source_name, stream->stream_name), frame);
                SVR_Stream_returnFrame(stream, frame);
            } else if(SVR_Stream_isOrphaned(stream)) {
                fprintf(stderr, "Stream \"%s\" orphaned\n", stream->source_name);
                cvDestroyWindow(Util_format("%s:%s", stream->source_name, stream->stream_name));
                Dictionary_remove(streams, stream->source_name);
                SVR_Stream_destroy(stream);
                list_stale = true;
            }
        }
    }

    if(stream_count == 0) {
        fprintf(stderr, "All streams orphaned\n");
        return -1;
    }

    return 0;
}
