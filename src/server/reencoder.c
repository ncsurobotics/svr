
#include <svr.h>
#include <svr/server/svr.h>

#include "reencoders/reencoders.h"

SVRs_Reencoder* SVRs_Reencoder_new(SVRs_Source* source, SVRs_Stream* stream) {
    SVRs_Reencoder* reencoder = malloc(sizeof(SVRs_Reencoder));
    
    reencoder->source = source;
    reencoder->stream = stream;
    reencoder->reencode = &SVRs_FullReencoder_reencode;

    return reencoder;
}

size_t SVRs_Reencoder_reencode(SVRs_Reencoder* reencoder, void* data, size_t n) {
    return reencoder->reencode(reencoder, data, n);
}
