#ifndef SOFDEC_MWS_CREATE_H
#define SOFDEC_MWS_CREATE_H

typedef struct MwsCreateParams {
    int file_type;
    int maximum_bps;
    int width;
    int height;
    int frame_count;
    int decoder_count;
    void* work;
    int work_size;
    int composition_mode;
    int buffer_format;
    int field_28;
    int field_2C;
} MwsCreateParams;

typedef char MwsCreateParamsSizeCheck[sizeof(MwsCreateParams) == 0x30 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

long MWSFTAG_IsUseAinfSj(const MwsCreateParams* create);

#ifdef __cplusplus
}
#endif

#endif
