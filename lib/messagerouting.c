/**
 * \file
 * \brief Message routing
 */

#include <svr.h>

typedef struct {
    const char* request_string;
    int (*callback)(SVR_Message* message);
} SVR_RequestMapping;

static SVR_RequestMapping request_types[] = {
    {"Stream.orphaned", SVR_MessageHandler_streamOrphaned},
    {"Data", SVR_MessageHandler_data},
    {"SVR.kick", SVR_MessageHandler_kick}
};

static int SVR_compareRequestMapping(const void* v1, const void* v2);
static SVR_RequestMapping* SVR_findRequestMapping(const char* request_string);

/**
 * \defgroup MessageRouting Message routing
 * \ingroup Comm
 * \brief Pass a message to an appropriate message handler
 * \{
 */

/**
 * Comparison used to sort the request mapping list
 */
static int SVR_compareRequestMapping(const void* v1, const void* v2) {
    return strcmp(((SVR_RequestMapping*)v1)->request_string, ((SVR_RequestMapping*)v2)->request_string);
}

/**
 * \brief Initialize the MessageRouter component
 *
 * Initialize the MessageRouter component
 */
void SVR_MessageRouter_init(void) {
    /* Sort request types so they can be efficiently searched */
    qsort(request_types, sizeof(request_types) / sizeof(request_types[0]), sizeof(request_types[0]), SVR_compareRequestMapping);
}

/**
 * Look up a request mapping for the given request string
 */
static SVR_RequestMapping* SVR_findRequestMapping(const char* request_string) {
    int lower = 0;
    int upper = (sizeof(request_types) / sizeof(request_types[0])) - 1;
    int sindex = 0;

    while(true) {
        while(request_types[lower].request_string[sindex] < request_string[sindex]) {
            lower++;

            if(lower > upper) {
                return NULL;
            }
        }

        while(request_types[upper].request_string[sindex] > request_string[sindex]) {
            upper--;

            if(lower > upper) {
                return NULL;
            }
        }

        if(request_string[sindex] == '\0') {
            return &request_types[lower];
        }

        sindex++;
    }

    return NULL;
}

/**
 * \brief Process a message
 *
 * Process an unsolicited message, i.e. a message that isn't a response to an
 * explicit request
 *
 * \param message The message to process
 * \return 0 and success, all other values indicate an error condition
 */
int SVR_MessageRouter_processMessage(SVR_Message* message) {
    SVR_RequestMapping* request_type;

    if(message->count == 0) {
        SVR_log(SVR_ERROR, "Received empty message");
        return -1;
    }

    request_type = SVR_findRequestMapping(message->components[0]);
    if(request_type == NULL) {
        SVR_log(SVR_ERROR, Util_format("Unsupported message type: %s", message->components[0]));
        return -1;
    }

    return request_type->callback(message);
}

/** \} */
