
#ifndef __SVR_ENCODING_INTERNAL_H
#define __SVR_ENCODING_INTERNAL_H

#include <svr.h>

/* Methods used by encodings, but not exposed as part of the public encoding API */

/* Provide encoded data for the encoder to buffer and provide via SVR_Encoder_readData */
void SVR_Encoder_provideData(SVR_Encoder* encoder, void* data, size_t n);

/* Provide the next n bytes of decoded frame data. Assumes required frame padding is already included */
void SVR_Decoder_writePaddedFrameData(SVR_Decoder* decoder, void* data, size_t n);
void SVR_Decoder_writeUnpaddedFrameData(SVR_Decoder* decoder, void* data, size_t n);
int SVR_Decoder_getRowPadding(SVR_Decoder* decoder);

#endif // #ifndef __SVR_ENCODING_INTERNAL_H
