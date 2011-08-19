
typedef struct Stream_s {
    Encoding* encoding;
    FrameProperties* frame_properties;
    void* encoder_state;
} Stream;
