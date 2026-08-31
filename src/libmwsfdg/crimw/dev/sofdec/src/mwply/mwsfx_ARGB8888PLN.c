#include "movie/mwsfx.h"

static const char invalid_handle[] =
    "E201195: mwPlyFxCnvFrmARGB8888: handle is invalid.";
static const char invalid_frame[] =
    "E201196: mwPlyFxCnvFrmARGB8888: getfrm is failed.";

void mwPlyFxCnvFrmARGB8888(int handle, MwsFrameInfo* frame, void* output) {
    SfxFrameInfo frame_info;
    SFXHandle* sfx_handle;

    if (MWSFD_IsEnableHndl(handle) == 0) {
        MWSFSVM_Error(invalid_handle);
    } else if (frame->frame == 0) {
        MWSFSVM_Error(invalid_frame);
    } else {
        sfx_handle = MWSFSFX_GetSfxHn(handle);
        MWSFSFX_CnvFrmInfToSfx(handle, frame, &frame_info);
        SFX_CnvFrmARGB8888(sfx_handle, &frame_info, output);
    }
}
