
#include "svr.h"
#include "svr/server/svr.h"

#include <signal.h>

static void SVRs_usage(const char* argv0);

void SVRs_exit(void) {
    exit(0);
}

void SVRs_exitError(void) {
    exit(-1);
}

static void SVRs_usage(const char* argv0) {
    printf("Usage: %s [-hd] [-b ADDRESS] [-l LOG_LEVEL] [-s SOURCES_CONFIG]\n"
           "Seawolf Video Router\n"
           "\n"
           "  -h                    Show this help message\n"
           "  -d                    Enable debugging\n"
           "  -b ADDRESS            Address to listen on\n"
           "  -l LOG_LEVEL          Log level (DEBUG, INFO, NORMAL, WARNING, ERROR, CRITICAL)\n"
           "  -s SOURCES_CONFIG     Sources configuration file\n", argv0);
}

int main(int argc, char** argv) {
    int opt;
    int debug_level = SVR_WARNING;
    char* source_conf_file = NULL;
    char* bind_address = "0.0.0.0";

    while((opt = getopt(argc, argv, ":hdl:s:b:")) != -1) {
        switch(opt) {
        case 'h':
            SVRs_usage(argv[0]);
            return 0;
        case 'd':
            debug_level = SVR_DEBUG;
            break;
        case 'b':
            bind_address = optarg;
            break;
        case 'l':
            debug_level = SVR_Logging_getLevelFromName(optarg);
            if(debug_level < 0) {
                fprintf(stderr, "Invalid logging level '%s'\n", optarg);
                SVRs_usage(argv[0]);
                return -1;
            }
            break;
        case 's':
            source_conf_file = optarg;
            break;
        case ':':
            fprintf(stderr, "Missing argument parameter\n");
            SVRs_usage(argv[0]);
            return -1;
        case '?':
            fprintf(stderr, "Unknown switch '%s'\n", argv[optind - 1]);
            SVRs_usage(argv[0]);
            return -1;
        }
    }

    SVR_initCore();
    SVR_Logging_setThreshold(debug_level);

    SVRs_Client_init();
    SVRs_Source_init();
    SVRs_MessageRouter_init();

    if(source_conf_file) {
        SVRs_Source_fromFile(source_conf_file);
    }

    /* Please *ignore* SIGPIPE. It will cause the program to close in the case
       of writing to a closed socket. We handle this ourselves. */
    signal(SIGPIPE, SIG_IGN);

    SVRs_Server_mainLoop(bind_address);
    
    return 0;
}
