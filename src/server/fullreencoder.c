
#include <svr.h>
#include <svr/server/svr.h>

size_t SVRs_FullReencoder_reencode(SVRs_Reencoder* reencoder, SVR_DataBuffer* data, size_t data_available, SVR_DataBuffer* target_buffer, size_t buffer_size) {
    IplImage* frame;

    frame = reencoder->source->encoding->decode(reencoder->source->decoder_data, data, data_available);

    if(frame) {
        return reencoder->stream->encoding->encode(reencoder->stream->encoder_data, frame, target_buffer, buffer_size);
    }

    return 0;
}
