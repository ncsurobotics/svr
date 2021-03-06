/**
 * \file net.c
 * \brief Message transmission/receipt
 */

#include "svr.h"

/**
 * \defgroup Net Message IO
 * \ingroup Comm
 * \brief Send and receive messages
 * \{
 */

/**
 * \brief Send a packed message
 *
 * Send a packed message over a socket
 *
 * \param socket Socket to send the message over
 * \param packed_message Packed message to send
 */
int SVR_Net_sendPackedMessage(int socket, SVR_PackedMessage* packed_message) {
    ssize_t sent_bytes, n;

    /* Send message body */
    sent_bytes = 0;
    while(sent_bytes < packed_message->length) {
        n = send(socket, ((uint8_t*)packed_message->data) + sent_bytes, packed_message->length - sent_bytes, 0);
        if(n < 0) {
            return n;
        }

        sent_bytes += n;
    }

    /* Send payload */
    sent_bytes = 0;
    while(sent_bytes < packed_message->payload_size) {
        n = send(socket, ((uint8_t*)packed_message->payload) + sent_bytes, packed_message->payload_size - sent_bytes, 0);
        if(n < 0) {
            return n;
        }

        sent_bytes += n;
    }

    return packed_message->length + packed_message->payload_size;
}

/**
 * \brief Send a message
 *
 * Send the message to the given client. The message will be packed using the
 * SVR_Arena associated with the original message
 *
 * \param socket The socket to use for IO
 * \param message The message to send
 */
int SVR_Net_sendMessage(int socket, SVR_Message* message) {
    SVR_PackedMessage* packed_message = SVR_Message_pack(message);
    return SVR_Net_sendPackedMessage(socket, packed_message);
}

/**
 * \brief Read from a socket
 *
 * Read data from a socket. This function is guaranteed to return the requested
 * number of bytes so long as no errors occur and the socket is not shut down.
 *
 * \param socket The socket to read from
 * \param buffer A pointer to a buffer to write to
 * \param len The amount of data to read
 * \param flags Additional flags as described in recv(2)
 * \return Return the number of bytes read (len) or -1 if an error occurs or 0
 * if the remote partner performs a shutdown
 */
static int SVR_Net_recv(int socket, void* buffer, size_t len, int flags) {
    int read = 0;
    int n;

    while(read < len) {
        n = recv(socket, ((char*)buffer) + read, len - read, flags | MSG_WAITALL);

        if(n <= 0) {
            return n;
        }

        read += n;
    }

    return read;
}

/**
 * \brief Receive a message
 *
 * Receive a message from the given socket. Will not retrieve the payload. If a
 * payload accompanies the message a call to SVR_Net_receivePayload should be
 * made after this call
 */
SVR_Message* SVR_Net_receiveMessage(int socket) {
    SVR_PackedMessage* packed_message;
    uint16_t message_length;
    ssize_t n = 0;

    /* The first byte received should be the size of the message that follows
       minus the header data */
    n = SVR_Net_recv(socket, &message_length, sizeof(uint16_t), MSG_PEEK);

    if(n <= 0) {
        return NULL;
    }

    /* Create space for the message and receive it */
    message_length = ntohs(message_length);
    packed_message = SVR_PackedMessage_new(message_length + SVR_MESSAGE_PREFIX_LEN);

    n = SVR_Net_recv(socket, packed_message->data, packed_message->length, 0);
    if(n <= 0) {
        SVR_PackedMessage_release(packed_message);
        return NULL;
    }

    return SVR_PackedMessage_unpack(packed_message);
}

/**
 * \brief Receive a message payload
 *
 * Receive the payload that follows a message. SVR_Net_receiveMessage does not
 * receive the payload to allow the caller to decide how to allocate space for
 * it. A call to SVR_Net_receivePayload *must* be made immediately after
 * receiving a message having a payload.
 *
 * \param socket The socket to receive from
 * \param message A payload of length message->payload_size will be stored to
 * message->payload
 * \return On success returns the number of bytes received, on failure returns
 * one of the return codes for the recv function
 */
int SVR_Net_receivePayload(int socket, SVR_Message* message) {
    return SVR_Net_recv(socket, message->payload, message->payload_size, 0);
}

/** \} */
