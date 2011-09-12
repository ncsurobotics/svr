
#ifndef __SVR_MESSAGE_H
#define __SVR_MESSAGE_H

/**
 * \brief An unpacked message
 *
 * The unpacked representation of a message. A message is associated with a
 * number of components consisting of ASCII text and request ID.
 */
struct SVR_Message_s {
    /**
     * A request ID for the message. A hub always returns responses to requests
     * using the same ID as given in the request. In this way responses can be
     * paired with requests. The ID 0 is reserved for messages not expecting
     * responses
     */
    uint16_t request_id;

    /**
     * Pointer to the payload
     */
    void* payload;
    
    /**
     * Payload size in bytes. A payload is appended to the message as part of the stream
     */
    uint16_t payload_size;

    /**
     * The components of the message. Each component is an ASCII string
     */
    char** components;

    /**
     * The number of components in the message
     */
    unsigned short count;

    /**
     * The Arena allocation that backs this message
     */
    SVR_Arena* alloc;
};

/**
 * \brief The packed representation of a message
 *
 * A messaged packed into a byte stream ready to be sent to the hub
 *
 * The packed message format is quite simple,<br>
 * <pre>
 *  length           [0:15]
 *  request id       [16:31]
 *  component count  [32:47]
 *  payload size     [48:63]
 *  data             [64:64 + length - payload size]
 *    component 1 \\0
 *    component 2 \\0
 *    ...
 *  payload          [64 + length - payload size + 1:64 + length]
 * </pre>
 *
 * The length, request ID, and component count constitute a 6 byte binary
 * header, and the rest of the message is null separated ASCII strings
 */
struct SVR_PackedMessage_s {
    size_t length;

    void* data;

    uint16_t payload_size;

    void* payload;

    /**
     * The Arena allocation that backs this message
     */
    SVR_Arena* alloc;
};

#define SVR_MESSAGE_PREFIX_LEN 4

void SVR_Message_init(void);

SVR_Message* SVR_Message_new(unsigned int component_count);
SVR_PackedMessage* SVR_Message_pack(SVR_Message* message);
void SVR_Message_release(SVR_Message* message);

SVR_PackedMessage* SVR_PackedMessage_new(size_t packed_length);
SVR_Message* SVR_PackedMessage_unpack(SVR_PackedMessage* packed_message);
void SVR_PackedMessage_release(SVR_PackedMessage* packed_message);


#endif // #ifndef __SVR_MESSAGE_H
