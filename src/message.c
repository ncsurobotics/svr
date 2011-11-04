
#include "svr.h"

static SVR_BlockAllocator* message_allocator = NULL;

static SVR_Message* SVR_Message_newWithAlloc(unsigned int component_count, SVR_Arena* alloc);
static SVR_PackedMessage* SVR_PackedMessage_newWithAlloc(size_t packed_length, SVR_Arena* alloc);

void SVR_Message_init(void) {
    message_allocator = SVR_BlockAlloc_newAllocator(256, 8);
}

/**
 * \brief Pack a message
 *
 * Return a packed message constructed from the given message
 *
 * \param message The message to packe
 * \return The packed equivalent of message
 */
SVR_PackedMessage* SVR_Message_pack(SVR_Message* message) {
    SVR_PackedMessage* packed_message;
    uint16_t total_data_length = 0;
    size_t pack_offset = 0;

    /* Add length of each component and space for a null terminator for each */
    for(int i = 0; i < message->count; i++) {
        total_data_length += strlen(message->components[i]) + 1;
    }

    /* Constructed the empty, packed message */
    packed_message = SVR_PackedMessage_newWithAlloc(total_data_length + SVR_MESSAGE_PREFIX_LEN, message->alloc);

    pack_offset = SVR_pack(packed_message->data, pack_offset, "hhhh", total_data_length, message->request_id, message->count, message->payload_size);
    for(int i = 0; i < message->count; i++) {
        pack_offset = SVR_pack(packed_message->data, pack_offset, "s", message->components[i]);
    }

    packed_message->payload = message->payload;
    packed_message->payload_size = message->payload_size;

    return packed_message;
}

/**
 * \brief Unpack a message
 *
 * Unpack and return the given packed message. The returned message can be freed
 * with a call to SVR_Message_destroyUnpacked()
 *
 * \param packed_message A packed message to unpack
 * \return The unpacked message
 */
SVR_Message* SVR_PackedMessage_unpack(SVR_PackedMessage* packed_message) {
    SVR_Message* message = SVR_Message_newWithAlloc(0, packed_message->alloc);
    size_t pack_offset = 0;
    uint16_t data_length;

    /* Read header */
    pack_offset = SVR_unpack(packed_message->data, pack_offset, "hhhh", &data_length, &message->request_id, &message->count, &message->payload_size);
    
    /* Store points to components (does not copy) */
    message->components = SVR_Arena_reserve(packed_message->alloc, sizeof(char*) * message->count);

    for(int i = 0; i < message->count; i++) {
        pack_offset = SVR_unpack(packed_message->data, pack_offset, "s", &message->components[i]);
    }

    return message;
}

/**
 * \brief Create a new message
 *
 * Create a new message with space for the given number of components. Space is
 * only allocated for the char pointers to the components, not to the
 * components themselves. Space for the components should be allocated and freed
 * separately.
 *
 * \param component_count The number of components to make space for. If component_count is 0, no allocation is done
 * \return A new message
 */
static SVR_Message* SVR_Message_newWithAlloc(unsigned int component_count, SVR_Arena* alloc) {
    SVR_Message* message = SVR_Arena_reserve(alloc, sizeof(SVR_Message));

    message->request_id = 0;
    message->count = component_count;
    message->components = NULL;
    message->payload = NULL;
    message->payload_size = 0;
    message->alloc = alloc;

    if(component_count) {
        message->components = SVR_Arena_reserve(alloc, sizeof(char*) * component_count);
    }

    return message;
}

SVR_Message* SVR_Message_new(unsigned int component_count) {
    SVR_Arena* alloc = SVR_Arena_alloc(message_allocator);
    return SVR_Message_newWithAlloc(component_count, alloc);
}

/**
 * \brief Create a new packed message object
 *
 * Return a new, emtpy packed message. No space is allocated to store data and
 * this should be allocated separately.
 *
 * \param alloc The SVR_MemPool allocation to allocate space for this message from
 * \return A new packed message object
 */
static SVR_PackedMessage* SVR_PackedMessage_newWithAlloc(size_t packed_length, SVR_Arena* alloc) {
    SVR_PackedMessage* packed_message = SVR_Arena_reserve(alloc, sizeof(SVR_PackedMessage));

    packed_message->data = SVR_Arena_reserve(alloc, packed_length);
    packed_message->length = packed_length;
    packed_message->payload = NULL;
    packed_message->payload_size = 0;
    packed_message->alloc = alloc;

    return packed_message;
}

SVR_PackedMessage* SVR_PackedMessage_new(size_t packed_length) {
    SVR_Arena* alloc = SVR_Arena_alloc(message_allocator);
    return SVR_PackedMessage_newWithAlloc(packed_length, alloc);
}

void SVR_PackedMessage_release(SVR_PackedMessage* packed_message) {
    SVR_Arena_free(packed_message->alloc);
}

void SVR_Message_release(SVR_Message* message) {
    SVR_Arena_free(message->alloc);
}
