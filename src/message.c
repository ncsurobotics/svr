
/**
 * \brief Pack a message
 *
 * Return a packed message constructed from the given message
 *
 * \param message The message to packe
 * \return The packed equivalent of message
 */
SVR_PackedMessage* SVR_packMessage(SVR_Message* message) {
    SVR_PackedMessage* packed_message = SVR_PackedMessage_newWithAlloc(message->alloc);
    size_t total_data_length = 0;
    uint32_t component_lengths = SVR_MemPool_reserve(message->alloc, sizeof(size_t) * message->count);
    char* buffer;
    int i;

    /* Add length of each message and space for a null terminator for each */
    for(i = 0; i < message->count; i++) {
        component_lengths[i] = strlen(message->components[i]) + 1;
        total_data_length += component_lengths[i];
    }

    total_data_length += message->payload_size;

    /* Store message information */
    packed_message->length = total_data_length + COMM_MESSAGE_PREFIX_LEN;
    packed_message->data = SVR_MemPool_reserve(message->alloc, packed_message->length);

    /* Build packed message header */
    ((uint16_t*)packed_message->data)[0] = htons(total_data_length);
    ((uint16_t*)packed_message->data)[1] = htons(message->request_id);
    ((uint16_t*)packed_message->data)[2] = htons(message->count);
    ((uint16_t*)packed_message->data)[3] = htons(message->payload_size);

    /* Copy message components */
    buffer = packed_message->data + COMM_MESSAGE_PREFIX_LEN;
    for(i = 0; i < message->count; i++) {
        memcpy(buffer, message->components[i], component_lengths[i]);
        buffer += component_lengths[i];
    }

    /* Copy payload */
    if(message->payload_size) {
        memcpy(buffer, message->payload, message->payload_size);
    }

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
SVR_Message* SVR_unpackMessage(SVR_PackedMessage* packed_message) {
    SVR_Message* message = SVR_Message_newWithAlloc(packed_message->alloc, 0);
    size_t data_length = ntohs(((uint16_t*)packed_message->data)[0]);

    /* Build message meta information */
    message->request_id = ntohs(((uint16_t*)packed_message->data)[1]);
    message->count = ntohs(((uint16_t*)packed_message->data)[2]);
    message->payload_size = ntohs(((uint16_t*)packed_message->data)[3]);

    /* Length of component strings part (data) */
    data_length -= message->payload_size;

    message->components = NULL;
    message->payload = NULL;

    if(message->count > 0) {
        message->components = SVR_MemPool_reserve(message->alloc, sizeof(char*) * message->count);

        /* Extract components -- we allocate all the space to the first and use the
           rest of the elements as indexes */
        message->components[0] = SVR_MemPool_reserve(message->alloc, data_length);
        memcpy(message->components[0], packed_message->data + COMM_MESSAGE_PREFIX_LEN, data_length);

        /* Point the rest of the components into the space allocated to the first */
        for(int i = 1; i < message->count; i++) {
            message->components[i] = message->components[i-1] + strlen(message->components[i-1]) + 1;
        }
    }

    if(message->payload_size > 0) {
        message->payload = SVR_MemPool_reserve(message->alloc, message->payload_size);
        memcpy(message->payload, packed_message->data + COMM_MESSAGE_PREFIX_LEN + data_length, message->payload_size);
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
 * \param alloc The SVR_MemPool allocation to allocate space for this message from
 * \param component_count The number of components to make space for. If component_count is 0, no allocation is done
 * \return A new message
 */
SVR_Message* SVR_Message_newWithAlloc(SVR_MemPool_Alloc* alloc, unsigned int component_count) {
    SVR_Message* message = SVR_MemPool_reserve(alloc, sizeof(SVR_Message));

    message->request_id = 0;
    message->count = component_count;
    message->components = NULL;
    message->payload = NULL;
    message->payload_size = 0;
    message->alloc = alloc;

    if(component_count) {
        message->components = SVR_MemPool_reserve(alloc, sizeof(char*) * component_count);
    }

    return message;
}

/**
 * \brief Create a new message
 *
 * Allocate and return a new message using a new allocation
 *
 * \param component_count The number of components to make space for
 * \return The newly allocated message
 * \see SVR_Message_newWithAlloc
 */
SVR_Message* SVR_Message_new(unsigned int component_count) {
    SVR_MemPool_Alloc* alloc = SVR_MemPool_alloc();
    return SVR_Message_newWithAlloc(alloc, component_count);
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
SVR_PackedMessage* SVR_PackedMessage_newWithAlloc(SVR_MemPool_Alloc* alloc) {
    SVR_PackedMessage* packed_message = SVR_MemPool_reserve(alloc, sizeof(SVR_PackedMessage));

    packed_message->length = 0;
    packed_message->data = NULL;
    packed_message->alloc = alloc;

    return packed_message;
}

/**
 * \brief Create a new packed message object
 *
 * Return a new packed message object allocated from a new SVR_MemPool allocation
 *
 * \return A new packed message object
 * \see SVR_PackedMessage_newWithAlloc
 */
SVR_PackedMessage* SVR_PackedMessage_new(void) {
    SVR_MemPool_Alloc* alloc = SVR_MemPool_alloc();
    return SVR_PackedMessage_newWithAlloc(alloc);
}

