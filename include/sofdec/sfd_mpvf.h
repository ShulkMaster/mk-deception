#ifndef MKD_SOFDEC_SFD_MPVF_H
#define MKD_SOFDEC_SFD_MPVF_H

#include "sofdec/mpv_mc.h"
#include "sofdec/sfd_player_types.h"
#include "sofdec/sfd_transport.h"

typedef struct SfdVideoFrameState SfdVideoFrameState;

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
    void* picture_user_buffer;
    union {
        unsigned char bytes[0x80];
        MPVPictureInfo decoded;
        struct {
            unsigned char reserved_58[0x14];
            int temporal_reference;
            unsigned char reserved_70[0x18];
            int decode_order;
            int field_order;
            unsigned char reserved_90[0x48];
        } order;
    } picture_info;
    int field_D8;
    int field_DC;
} SfdMpvFrame;

typedef struct SfdMpvFrameWork {
    MPVContext* decoder;
    int setup_values[9];
    int saved_pair[2];
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
    int reserved_17C;
    SfdMpvFrame frames[16];
} SfdMpvFrameWork;

typedef char SfdMpvFrameSizeCheck[sizeof(SfdMpvFrame) == 0xE0 ? 1 : -1];
typedef char SfdMpvFrameWorkSizeCheck[
    sizeof(SfdMpvFrameWork) == 0xF80 ? 1 : -1];

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
                                 const void* frame_data);

#endif
