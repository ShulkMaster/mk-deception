#ifndef MKD_SOFDEC_SFD_HEADER_H
#define MKD_SOFDEC_SFD_HEADER_H

#include "sofdec/sfd_player_types.h"

typedef struct SfdHeaderAudioInfo {
    int codec;
    int layer;
    int channel_count;
    int sample_rate;
} SfdHeaderAudioInfo;

typedef struct SfdHeaderVideoInfo {
    int codec;
    int bit_rate;
    int width;
    int height;
    int picture_rate;
    int has_effective_features;
    int color_type;
    int picture_type;
    int fixed_flag;
    int sequence_header_fixed_flag;
    int expand;
    int gop_n;
    int gop_m;
} SfdHeaderVideoInfo;

/* SFHDS_FHD: parsed header fields followed by the raw 0x800-byte packet. */
typedef struct SfdHeaderState {
    int processed;
    int mux_version_major;
    int mux_version_minor;
    int byte_rate;
    int header_size;
    int pack_type;
    int packet_size_length;
    int pack_size;
    int element_count;
    int audio_element_count;
    int video_element_count;
    int private_element_count;
    int maximum_audio_length;
    int maximum_video_length;
    int maximum_frame_count;
    int private_stream_1;
    int private_stream_2;
    int audio_stream;
    int video_stream;
    SfdHeaderAudioInfo audio;
    SfdHeaderVideoInfo video;
    int raw_header_size;
    unsigned char raw_header[0x800];
} SfdHeaderState;

typedef char SfdHeaderStateSizeCheck[
    sizeof(SfdHeaderState) == 0x894 ? 1 : -1];

void SFHDS_InitFhd(SfdHeaderState* state, int enabled);
void SFHDS_FinishFhd(SfdHeaderState* state);
int SFHDS_SetHdr(SfdHandle* handle, int stream_index,
                 const unsigned char* data, int size, int* header_flag);
int SFHDS_GetColType(SfdHandle* handle);
void SFHDS_Init(void);
void SFHDS_Finish(void);

#endif
