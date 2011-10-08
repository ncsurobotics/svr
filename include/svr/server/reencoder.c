
#include <svr.h>
#include <svr/server/svr.h>

SVRs_Reencoder* SVRs_Reencoder_new(SVRs_Source* source, SVRs_Stream* stream) {
    SVRs_Reencoder* reencoder = malloc(sizeof(SVRs_Reencoder));
    
    reencoder->source = source;
    reencoder->stream = stream;
    reencoder->reencoder = &SVRs_FullReencoder_reencode;
}
