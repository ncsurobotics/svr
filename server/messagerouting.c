
#include "svr.h"
#include "svrd.h"

/*
 * Messages
 *
 * Stream.{open,close,setProp,getProp}
 * Source.{open,close,setProp,getProp}
 * Data
 * Event.{register,unregister,notify}
 * SVR.{kick,response}
 */

typedef struct {
    const char* request_string;
    void (*callback)(SVRD_Client* client, SVR_Message* message);
} SVRD_RequestMapping;

static const char* INVALID_MESSAGE = "Invalid message";
static SVRD_RequestMapping request_types[] = {
    {"Stream.open", SVRD_Stream_rOpen},
    {"Stream.close", SVRD_Stream_rClose},
    {"Stream.attachSource", SVRD_Stream_rAttachSource},
    {"Stream.resize", SVRD_Stream_rResize},
    {"Stream.setChannels", SVRD_Stream_rSetChannels},
    {"Stream.setEncoding", SVRD_Stream_rSetEncoding},
    {"Stream.setDropRate", SVRD_Stream_rSetDropRate},
    {"Stream.setPriority", SVRD_Stream_rSetPriority},
    {"Stream.getInfo", SVRD_Stream_rGetInfo},
    {"Stream.pause", SVRD_Stream_rPause},
    {"Stream.unpause", SVRD_Stream_rUnpause},

    {"Source.open", SVRD_Source_rOpen},
    {"Source.setEncoding", SVRD_Source_rSetEncoding},
    {"Source.setFrameProperties", SVRD_Source_rSetFrameProperties},
    {"Source.close", SVRD_Source_rClose},
    {"Source.getSourcesList", SVRD_Source_rGetSourcesList},
    {"Data", SVRD_Source_rData},

    {"Event.register", SVRD_Event_rRegister},
    {"Event.unregister", SVRD_Event_rUnregister}
};

static int SVRD_compareRequestMapping(const void* v1, const void* v2);
static SVRD_RequestMapping* SVRD_findRequestMapping(const char* request_string);

static int SVRD_compareRequestMapping(const void* v1, const void* v2) {
    return strcmp(((SVRD_RequestMapping*)v1)->request_string, ((SVRD_RequestMapping*)v2)->request_string);
}

void SVRD_MessageRouter_init(void) {
    /* Sort request types so they can be efficiently searched */
    qsort(request_types, sizeof(request_types) / sizeof(request_types[0]), sizeof(request_types[0]), SVRD_compareRequestMapping);
}

static SVRD_RequestMapping* SVRD_findRequestMapping(const char* request_string) {
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

void SVRD_processMessage(SVRD_Client* client, SVR_Message* message) {
    SVRD_RequestMapping* request_type;

    if(message->count == 0) {
        SVRD_Client_kick(client, INVALID_MESSAGE);
        return;
    }

    request_type = SVRD_findRequestMapping(message->components[0]);
    if(request_type == NULL) {
        SVRD_Client_kick(client, INVALID_MESSAGE);
        return;
    }

    request_type->callback(client, message);
}
