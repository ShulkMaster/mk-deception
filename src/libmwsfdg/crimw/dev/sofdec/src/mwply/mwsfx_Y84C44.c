#include "movie/mwsfx.h"

static const char invalid_handle[] =
    "E201199: mwPlyFxCnvFrmYUV422: handle is invalid.";
static const char invalid_frame[] =
    "E2011910: mwPlyFxCnvFrmYUV422: getfrm is failed.";

void mwPlyFxCnvFrmY84C44(int handle, MwsFrameInfo* frame, void* y, void* c) {
    SfxFrameInfo frame_info;
    SFXHandle* sfx_handle;

    if (MWSFD_IsEnableHndl(handle) == 0) {
        MWSFSVM_Error(invalid_handle);
    } else if (frame->frame == 0) {
        MWSFSVM_Error(invalid_frame);
    } else {
        sfx_handle = MWSFSFX_GetSfxHn(handle);
        MWSFSFX_CnvFrmInfToSfx(handle, frame, &frame_info);
        SFX_CnvFrmY84C44(sfx_handle, &frame_info, y, c);
    }
}
