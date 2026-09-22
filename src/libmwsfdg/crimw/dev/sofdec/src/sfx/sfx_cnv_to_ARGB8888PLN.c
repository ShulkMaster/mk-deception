#include "sofdec/sfx.h"

void SFX_CnvFrmARGB8888(SFXHandle* handle, SFXFrameInfo* frame,
                        void* output)
{
    int format = frame->format;

    if (handle->composition_mode == 0) {
        handle->composition_mode = SFXINF_GetStmInf(frame, "COMPO");
    }
    switch (format) {
    case 3:
        SFX_CnvFrmYcc420plnToArgb8888(handle, frame, output);
        break;
    default:
        SFXLIB_Error(
            handle, frame,
            "E201181: SFX_CnvFrmArgb8888 : frmfmt is not support.");
        break;
    }
}
