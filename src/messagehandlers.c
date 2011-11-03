
#include <svr.h>

int SVR_MessageHandler_data(SVR_Message* message) {
    const char* stream_name;

    if(message->count != 2 || message->payload_size == 0 || strcmp(message->components[0], "Data") != 0) {
        return -1;
    }

    stream_name = message->components[1];
    SVR_Stream_provideData(stream_name, message->payload, message->payload_size);
    return 0;
}
