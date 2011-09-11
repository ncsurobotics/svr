
#ifndef __SVR_REENCODER_H
#define __SVR_REENCODER_H

#include <svr/forward.h>

/* Types,
 *  FullReencoder (do a full decode-encode)
 *  DirectCopyReencoder (raw bytes in and out)
 *  FFV1Reencoder (handle efficiently reencoding interframe to equivalent interframe)
 *  etc.
 */

struct Reencoder_s {
    /**
     * Source needed for frame properties and encoding
     */
    Source* source;
    
    /**
     * Stream needed for frame properties and encoding
     */
    Stream* stream;

    /**
     * Convert data provided by the source into data needed by the stream.
     * Return number of bytes written 
     */
    size_t (*reencode)(Reencoder* reencoder, DataBuffer* data, size_t data_available, DataBuffer* target_buffer, size_t buffer_size);
};

/**
 * Create a new reencoder using the most ideal reencoding mechanism for
 * adapting source to stream
 */
Reencoder* Reencoder_new(Source* source, Stream* stream);

#endif // #ifndef __SVR_REENCODER_H
