
#ifndef __SVR_FORWARDDECLARATIONS_H
#define __SVR_FORWARDDECLARATIONS_H

typedef uint8_t DataBuffer;

struct SVR_MemPool_s;
struct SVR_MemPool_Block_s;
struct SVR_Arena_s;
struct BlockAllocator_s;
struct StreamBufferPool_s;
struct StreamBuffer_s;
struct SVR_Message_s;
struct SVR_PackedMessage_s;
struct Client_s;
struct Encoding_s;
struct Reencoder_s;
struct Stream_s;
struct Source_s;
struct FrameProperties_s;

typedef struct SVR_MemPool_s SVR_MemPool;
typedef struct SVR_MemPool_Block_s SVR_MemPool_Block;
typedef struct SVR_Arena_s SVR_Arena;
typedef struct StreamBuffer_s StreamBuffer;
typedef struct StreamBufferPool_s StreamBufferPool;
typedef struct BlockAllocator_s BlockAllocator;
typedef struct SVR_Message_s SVR_Message;
typedef struct SVR_PackedMessage_s SVR_PackedMessage;
typedef struct Client_s Client;
typedef struct Encoding_s Encoding;
typedef struct Reencoder_s Reencoder;
typedef struct Stream_s Stream;
typedef struct Source_s Source;
typedef struct FrameProperties_s FrameProperties;

#endif // #ifndef __SVR_FORWARDDECLARATIONS_H
