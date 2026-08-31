#include "sofdec/sfx.h"

void SFX_CnvFrmARGB8888(SFXHandle* handle, SFXFrameInfo* frame,
                        void* output)
{
    int format = frame->format;

    if (handle->stream_info == 0) {
        handle->stream_info = SFXINF_GetStmInf(frame, "COMPO");
    }
    switch (format) {
    case 3:
        SFX_CnvFrmYcc420plnToArgb8888(handle, frame, output);
        break;
    default:
        SFXLIB_Error(
            handle, (void*)frame,
            "E201181: SFX_CnvFrmArgb8888 : frmfmt is not support.");
        break;
    }
}
