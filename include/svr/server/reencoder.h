
#ifndef __SVR_REENCODER_H
#define __SVR_REENCODER_H

#include <svr/forward.h>
#include <svr/server/forward.h>

/* Types,
 *  FullReencoder (do a full decode-encode)
 *  DirectCopyReencoder (raw bytes in and out)
 *  FFV1Reencoder (handle efficiently reencoding interframe to equivalent interframe)
 *  etc.
 */

struct SVRs_Reencoder_s {
    /**
     * Source needed for frame properties and encoding
     */
    SVRs_Source* source;
    
    /**
     * Stream needed for frame properties and encoding
     */
    SVRs_Stream* stream;

    /**
     * Convert data provided by the source into data needed by the stream.
     * Return number of bytes written 
     */
    size_t (*reencode)(SVRs_Reencoder* reencoder, void* data, size_t data_available, void* target_buffer, size_t buffer_size);
};

/**
 * Create a new reencoder using the most ideal reencoding mechanism for
 * adapting source to stream
 */
SVRs_Reencoder* SVRs_Reencoder_new(SVRs_Source* source, SVRs_Stream* stream);

size_t SVRs_FullReencoder_reencode(SVRs_Reencoder* reencoder, void* data, size_t data_available, void* target_buffer, size_t buffer_size);

#endif // #ifndef __SVR_REENCODER_H
