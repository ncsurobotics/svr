
#include <svr.h>
#include <svr/server/svr.h>

/* source_name stream_name [encoding] */
void SVRs_Stream_rOpen(SVRs_Client* client, SVR_Message* message) {
    SVR_Encoding* encoding = NULL;
    SVRs_Source* source;
    char* source_name;
    char* stream_name;
    char* encoding_name = NULL;

    switch(message->count) {
    case 4:
        encoding_name = message->components[3];
    case 3:
        source_name = message->components[1];
        stream_name = message->components[2];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    source = SVRs_getSourceByName(source_name);
    if(source == NULL) {
        SVRs_Client_replyError(client, message, SVR_NOSUCHSOURCE);
        return;
    }

    if(Dictionary_exists(client->streams, stream_name)) {
        SVRs_Client_replyError(client, message, SVR_NAMECLASH);
        return;
    }

    if(encoding_name) {
        encoding = SVR_getEncoding(encoding_name);
        if(encoding == NULL) {
            SVRs_Client_replyError(client, message, SVR_NOSUCHENCODING);
            return;
        }
    }

    SVRs_Stream_open(client, source, encoding, stream_name);
    SVRs_Client_replySuccess(client, message);
}

/* stream_name */
void SVRs_Stream_rClose(SVRs_Client* client, SVR_Message* message) {
    SVRs_Stream* stream;
    char* stream_name;

    switch(message->count) {
    case 2:
        stream_name = message->components[1];
        break;

    default:
        SVRs_Client_kick(client, "Invalid message");
        return;
    }

    stream = Dictionary_get(client->streams, stream_name);
    if(stream == NULL) {
        SVRs_Client_replyError(client, message, SVR_NOSUCHSTREAM);
        return;
    }

    SVRs_Stream_close(stream);
}

void SVRs_Stream_rGetProp(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Stream_rSetProp(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Source_rOpen(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Source_rClose(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Source_rGetProp(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Source_rSetProp(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Source_rData(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Event_rRegister(SVRs_Client* client, SVR_Message* message) {
    // --
}

void SVRs_Event_rUnregister(SVRs_Client* client, SVR_Message* message) {
    // --
}
