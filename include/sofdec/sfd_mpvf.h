#ifndef MKD_SOFDEC_SFD_MPVF_H
#define MKD_SOFDEC_SFD_MPVF_H

#include "sofdec/mpv_mc.h"
#include "sofdec/sfd_player_types.h"
#include "sofdec/sfd_transport.h"

typedef struct SfdVideoFrameState SfdVideoFrameState;
typedef struct SfdMpvPictureUserData SfdMpvPictureUserData;

typedef struct SfdMpvParameters {
    int chroma_width;
    int chroma_height;
    int width;
    int height;
    int reference_buffer;
    int maximum_width;
    int maximum_height;
    int frame_count;
    int frame_buffer;
} SfdMpvParameters;

typedef char SfdMpvParametersSizeCheck[
    sizeof(SfdMpvParameters) == 0x24 ? 1 : -1];

typedef struct SfdMpvFrame {
    int state;
    int reference_count;
    void* frame_buffer;
    SfdTimerTimeUnit time_unit;
    int display_time_value;
    int display_time_scale;
    int field_40;
    int field_44;
    int picture_order;
    int field_4C;
    int field_50;
    SfdMpvPictureUserData* picture_user_buffer;
    MPVPictureInfo picture_info;
    long long picture_pts;
} SfdMpvFrame;

typedef struct SfdMpvFrameWork {
    MPVContext* decoder;
    SfdMpvParameters setup_values;
    void* saved_pair[2];
    void* address_table[16];
    SfdMpvFrame* active_frame;
    int decode_state;
    int decode_mode;
    int decoder_terminated;
    int gop_state;
    int field_084;
    int field_088;
    MPVPictureInfo picture_info;
    int field_10C;
    int field_110;
    int active_size_threshold;
    int field_118;
    int field_11C;
    int field_120;
    int reserved_124;
    SfdPtsEntry pts_entry;
    int plane_indices[2];
    MPVPlaneSet planes[2];
    SfdMpvFrame* reference_frames[2];
    SfdMpvFrame* pending_frame;
    int frame_state[3];
    int frame_count;
    int reserved_17C; /* 8-byte alignment for frames at +0x180. */
    SfdMpvFrame frames[16];
} SfdMpvFrameWork;

typedef char SfdMpvFrameSizeCheck[sizeof(SfdMpvFrame) == 0xE0 ? 1 : -1];
typedef char SfdMpvFrameWorkSizeCheck[
    sizeof(SfdMpvFrameWork) == 0xF80 ? 1 : -1];

void SFD_SetMpvParaTbl(SfdMpvParameters* parameters,
                       void** reference_buffers,
                       void** frame_buffers);
int SFMPVF_IsNextFrmReady(SfdHandle* handle);
SfdMpvFrame* SFMPVF_HoldFrm(SfdHandle* handle, int* sole_frame);
void SFMPVF_EndRefFrm(SfdMpvFrame* frame);
void SFMPVF_EndDrawFrm(SfdMpvFrame* frame);
void SFMPVF_RefStbyFrm(SfdMpvFrame* frame);
void SFMPVF_StbyFrm(SfdMpvFrame* frame);
void SFMPVF_FreeFrm(SfdMpvFrame* frame);
SfdMpvFrame* SFMPVF_AllocFrm(SfdHandle* handle);
int SFMPVF_GetNumFrm(SfdHandle* handle);
void SFMPVF_SetGopStat(SfdHandle* handle, int state);
int SFMPVF_IsTermDec(SfdHandle* handle);
void SFMPVF_TermDec(SfdHandle* handle);
SfdVideoFrameState* SFMPVF_SearchVfrmData(SfdHandle* handle,
                                          const SfdMpvFrame* frame);
SfdMpvFrame* SFMPVF_SearchFrmObj(SfdHandle* handle,
                                 const SfdVideoFrameInfo* info);

#endif
