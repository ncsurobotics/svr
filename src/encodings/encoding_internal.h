
#ifndef __SVR_ENCODING_INTERNAL_H
#define __SVR_ENCODING_INTERNAL_H

#include <svr.h>

/* Methods used by encodings, but not exposed as part of the public encoding API */

/* Provide encoded data for the encoder to buffer and provide via SVR_Encoder_readData */
void SVR_Encoder_provideData(SVR_Encoder* encoder, void* data, size_t n);

/* Get the frame currently being built/decoded */
IplImage* SVR_Decoder_getCurrentFrame(SVR_Decoder* decoder);

/* Signal that the current frame is completely decoded */
void SVR_Decoder_currentFrameComplete(SVR_Decoder* decoder);

#endif // #ifndef __SVR_ENCODING_INTERNAL_H
