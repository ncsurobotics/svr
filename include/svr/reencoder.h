
/* Types,
 *  FullReencoder (do a full decode-encode)
 *  DirectCopyReencoder (raw bytes in and out)
 *  FFV1Reencoder (handle efficiently reencoding interframe to equivalent interframe)
 *  etc.
 */

typedef struct Reencoder_s {
    void (*reencode)(Source* source, Stream* dest);
} Reencoder;
