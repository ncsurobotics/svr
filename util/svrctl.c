
#include <svr.h>
#include <getopt.h>

enum svrctl_job_type {
    SVRCTL_OPEN,
    SVRCTL_CLOSE,
    SVRCTL_CLOSEALL,
    SVRCTL_LISTALL
};

struct svrctl_job {
    enum svrctl_job_type type;
    char* arg0;
    char* arg1;
};

static void svrctl_usage(const char* argv0);

static void svrctl_usage(const char* argv0) {
    printf("Usage: %s [-hd] [-s ADDRESS] [-o NAME,SOURCE_DESCRIPTOR] [-c NAME] [--close-all] [--list-all]\n"
           "Seawolf Video Router Control\n"
           "\n"
           "  -h, --help                            Show this help message\n"
           "  -d, --debug                           Enable debugging\n"
           "  -s, --server=ADDRESS                  Address of SVR server\n"
           "  -o, --open NAME,SOURCE_DESCRIPTOR     Open a new server source\n"
           "  -c, --close NAME                      Close a server source\n"
           "      --close-all                       Close all server sources\n"
           "      --list-all                        List all sources\n\n", argv0);
}

int main(int argc, char** argv) {
    int opt, indexptr, i, err;
    char* source_name;
    List* sources;

    struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"server", 1, NULL, 's'},
        {"open", 1, NULL, 'o'},
        {"close", 1, NULL, 'c'},
        {"close-all", 0, NULL, 'C'},
        {"list-all", 0, NULL, 'L'},
        {NULL, 0, NULL, 0}
    };

    struct svrctl_job* jobs = NULL;
    int job_count = 0;

    SVR_Logging_setThreshold(SVR_LOGGING_OFF);

    while((opt = getopt_long(argc, argv, ":hds:o:c:", long_options, &indexptr)) != -1) {
        switch(opt) {
        case 'h':
            svrctl_usage(argv[0]);
            return 0;

        case 'd':
            SVR_Logging_setThreshold(SVR_DEBUG);
            break;

        case 's':
            SVR_setServerAddress(optarg);
            break;

        case 'o':
            jobs = realloc(jobs, sizeof(struct svrctl_job) * (job_count + 1));
            jobs[job_count].type = SVRCTL_OPEN;
            jobs[job_count].arg0 = optarg;

            i = 0;
            while(strchr(",", jobs[job_count].arg0[i]) == NULL) {
                i++;
            }

            if(jobs[job_count].arg0[i] == '\0') {
                fprintf(stderr, "Invalid argument to --open\n\n");
                svrctl_usage(argv[0]);
                return -1;
            }
            
            jobs[job_count].arg0[i] = '\0';
            jobs[job_count].arg1 = jobs[job_count].arg0 + i + 1;
            job_count++;

            break;

        case 'c':
            jobs = realloc(jobs, sizeof(struct svrctl_job) * (job_count + 1));
            jobs[job_count].type = SVRCTL_CLOSE;
            jobs[job_count++].arg0 = optarg;
            break;

        case 'C':
            jobs = realloc(jobs, sizeof(struct svrctl_job) * (job_count + 1));
            jobs[job_count++].type = SVRCTL_CLOSEALL;
            break;

        case 'L':
            jobs = realloc(jobs, sizeof(struct svrctl_job) * (job_count + 1));
            jobs[job_count++].type = SVRCTL_LISTALL;
            break;

        case ':':
            fprintf(stderr, "Missing argument parameter\n\n");
            svrctl_usage(argv[0]);
            return -1;

        case '?':
            fprintf(stderr, "Unknown switch '%s'\n\n", argv[optind - 1]);
            svrctl_usage(argv[0]);
            return -1;

        default:
            break;
        }
    }

    if(job_count == 0) {
        svrctl_usage(argv[0]);
        return 0;
    }

    err = SVR_init();
    if(err) {
        fprintf(stderr, "Could not connect to SVR server!\n");
        return -1;
    }

    for(i = 0; i < job_count; i++) {
        switch(jobs[i].type) {
        case SVRCTL_OPEN:
            err = SVR_openServerSource(jobs[i].arg0, jobs[i].arg1);

            switch(err) {
            case SVR_SUCCESS:
                break;

            case SVR_NAMECLASH:
                fprintf(stderr, "Source '%s' already exists\n", jobs[i].arg0);
                break;

            case SVR_INVALIDARGUMENT:
                fprintf(stderr, "Invalid source type for '%s'\n", jobs[i].arg0);
                break;

            case SVR_PARSEERROR:
                fprintf(stderr, "Invalid source descriptor for '%s'\n", jobs[i].arg0);
                break;
                
            default:
                fprintf(stderr, "Uknown error opening '%s'\n", jobs[i].arg0);
                break;
            }
            break;

        case SVRCTL_CLOSE:
            err = SVR_closeServerSource(jobs[i].arg0);
            switch(err) {
            case SVR_SUCCESS:
                break;
                
            case SVR_NOSUCHSOURCE:
                fprintf(stderr, "Source '%s' does not exist\n", jobs[i].arg0);
                break;
                
            case SVR_INVALIDARGUMENT:
                fprintf(stderr, "Source '%s' is not a server source\n", jobs[i].arg0);
                break;
                
            default:
                fprintf(stderr, "Uknown error closing '%s'\n", jobs[i].arg0);
                break;
            }
            break;

        case SVRCTL_CLOSEALL:
            sources = SVR_getSourcesList();
            List_sort(sources, List_compareString);

            for(int i = 0; (source_name = List_get(sources, i)) != NULL; i++) {
                if(source_name[0] == 's') {
                    SVR_closeServerSource(source_name + 2);
                }
            }

            SVR_freeSourcesList(sources); 
            break;

        case SVRCTL_LISTALL:
            sources = SVR_getSourcesList();
            List_sort(sources, List_compareString);

            for(int i = 0; (source_name = List_get(sources, i)) != NULL; i++) {
                printf("%s\n", source_name);
            }

            SVR_freeSourcesList(sources);
            break;
        }
    }

    free(jobs);
    
    return 0;
}
