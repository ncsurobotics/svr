/**
 * \file
 * \brief Message type handlers
 */

#include <svr.h>

/**
 * \defgroup MessageHandlers Message handlers
 * \ingroup Comm
 * \brief Functions to handle different types of received messages
 * \{
 */

/**
 * \brief Process a "stream orphaned" message
 *
 * Process a stream orphaned message to mark the given stream as orphaned
 *
 * \param message Message to process
 * \return 0 on success, -1 otherwise
 */
int SVR_MessageHandler_streamOrphaned(SVR_Message* message) {
    const char* stream_name;

    if(message->count != 2) {
        return -1;
    }

    stream_name = message->components[1];
    SVR_Stream_setOrphaned(stream_name);
    return 0;
}

/**
 * \brief Process a "data" message
 *
 * Process a data message which provides frame data for a stream
 *
 * \param message Message to process
 * \return 0 on success, -1 otherwise
 */
int SVR_MessageHandler_data(SVR_Message* message) {
    const char* stream_name;

    if(message->count != 2 || message->payload_size == 0 || strcmp(message->components[0], "Data") != 0) {
        return -1;
    }

    stream_name = message->components[1];
    SVR_Stream_provideData(stream_name, message->payload, message->payload_size);
    return 0;
}

/**
 * \brief Process a "kick" message
 *
 * Process a kick message sent by the server when the client is being forcefully
 * dropped
 *
 * \param message Message to process
 * \return 0 on success, -1 otherwise
 */
int SVR_MessageHandler_kick(SVR_Message* message) {
    SVR_log(SVR_CRITICAL, "Kicked from SVR server!");
    return 0;
}

/** \} */
