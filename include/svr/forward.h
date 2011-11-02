
#ifndef __SVR_FORWARDDECLARATIONS_H
#define __SVR_FORWARDDECLARATIONS_H

struct SVR_MemPool_s;
struct SVR_MemPool_Block_s;
struct SVR_Arena_s;
struct SVR_BlockAllocator_s;
struct SVR_RefCounter_s;
struct SVR_Message_s;
struct SVR_PackedMessage_s;
struct SVR_Encoding_s;
struct SVR_Encoder_s;
struct SVR_Decoder_s;
struct SVR_Stream_s;
struct SVR_FrameProperties_s;

typedef struct SVR_MemPool_s SVR_MemPool;
typedef struct SVR_MemPool_Block_s SVR_MemPool_Block;
typedef struct SVR_Arena_s SVR_Arena;
typedef struct SVR_BlockAllocator_s SVR_BlockAllocator;
typedef struct SVR_RefCounter_s SVR_RefCounter;
typedef struct SVR_Message_s SVR_Message;
typedef struct SVR_PackedMessage_s SVR_PackedMessage;
typedef struct SVR_Encoding_s SVR_Encoding;
typedef struct SVR_Encoder_s SVR_Encoder;
typedef struct SVR_Decoder_s SVR_Decoder;
typedef struct SVR_Stream_s SVR_Stream;
typedef struct SVR_FrameProperties_s SVR_FrameProperties;

#endif // #ifndef __SVR_FORWARDDECLARATIONS_H
