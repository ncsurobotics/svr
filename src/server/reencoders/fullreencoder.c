
#include <svr.h>
#include <svr/server/svr.h>

#include "reencoders.h"

size_t SVRs_FullReencoder_reencode(SVRs_Reencoder* reencoder, void* data, size_t data_available) {
    IplImage* frame;

    SVR_Decoder_decode(reencoder->source->decoder, data, data_available);

    while(SVR_Decoder_framesReady(reencoder->source->decoder)) {
        frame = SVR_Decoder_getFrame(reencoder->source->decoder);
        SVR_Encoder_encode(reencoder->stream->encoder, frame);
        SVR_Decoder_returnFrame(reencoder->source->decoder, frame);
    }

    return SVR_Encoder_dataReady(reencoder->stream->encoder);
}
