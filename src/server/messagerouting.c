
#include "svr.h"
#include "svr/server/svr.h"

/*
 * Messages
 *
 * Stream.{open,close,setProp,getProp}
 * Source.{open,close,setProp,getProp}
 * Data
 * Event.{register,unregister,notify}
 * SVR.{kick}
 */

typedef struct {
    const char* request_string;
    void (*callback)(SVRs_Client* client, SVR_Message* message);
} SVRs_RequestMapping;

static const char* INVALID_MESSAGE = "Invalid message";
static SVRs_RequestMapping request_types[] = {
    {"Stream.open", SVRs_Stream_open},
    {"Stream.close", SVRs_Stream_close},
    {"Stream.getProp", SVRs_Stream_getProp},
    {"Stream.setProp", SVRs_Stream_setProp},
    {"Source.open", SVRs_Source_open},
    {"Source.close", SVRs_Source_close},
    {"Source.getProp", SVRs_Source_getProp},
    {"Source.setProp", SVRs_Source_setProp},
    {"Data", SVRs_Source_data},
    {"Event.register", SVRs_Event_register},
    {"Event.unregister", SVRs_Event_unregister}
};

static int SVRs_compareRequestMapping(const void* v1, const void* v2);
static SVRs_RequestMapping* SVRs_findRequestMapping(const char* request_string);

static int SVRs_compareRequestMapping(const void* v1, const void* v2) {
    return strcmp(((SVRs_RequestMapping*)v1)->request_string, ((SVRs_RequestMapping*)v2)->request_string);
}

void SVRs_MessageRouter_init(void) {
    /* Sort request types so they can be efficiently searched */
    qsort(request_types, sizeof(request_types) / sizeof(request_types[0]), sizeof(request_types[0]), SVRs_compareRequestMapping);
}

static SVRs_RequestMapping* SVRs_findRequestMapping(const char* request_string) {
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

void SVRs_processMessage(SVRs_Client* client, SVR_Message* message) {
    SVRs_RequestMapping* request_type;
    
    if(message->count == 0) {
        SVRs_Client_kick(client, INVALID_MESSAGE);
        return;
    }

    request_type = SVRs_findRequestMapping(message->components[0]);
    if(request_type == NULL) {
        SVRs_Client_kick(client, INVALID_MESSAGE);
        return;
    }

    request_type->callback(client, message);
}
