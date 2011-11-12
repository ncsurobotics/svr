
#include <svr.h>

int SVR_MessageHandler_streamOrphaned(SVR_Message* message) {
    const char* stream_name;

    if(message->count != 2) {
        return -1;
    }

    stream_name = message->components[1];
    SVR_Stream_setOrphaned(stream_name);
    return 0;
}

int SVR_MessageHandler_data(SVR_Message* message) {
    const char* stream_name;

    if(message->count != 2 || message->payload_size == 0 || strcmp(message->components[0], "Data") != 0) {
        return -1;
    }

    stream_name = message->components[1];
    SVR_Stream_provideData(stream_name, message->payload, message->payload_size);
    return 0;
}

int SVR_MessageHandler_kick(SVR_Message* message) {
    SVR_log(SVR_CRITICAL, "Kicked from SVR server!");
    return 0;
}
