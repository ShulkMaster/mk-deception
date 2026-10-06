#ifndef MKD_SOFDEC_MPV_ABDEC_H
#define MKD_SOFDEC_MPV_ABDEC_H

#include "dolphin/types.h"

s32 MPVABDEC_NintraBlock(void* context, void* coding_block);
s32 MPVABDEC_IntraBlock(void* context, void* coding_block);
s32 MPVABDEC_IntraBlockDc11(void* context, void* coding_block);

#endif
