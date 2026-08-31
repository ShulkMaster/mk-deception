#include "sofdec/sfx.h"

void SFX_CnvFrmY84C44(SFXHandle* handle, SFXFrameInfo* frame,
                       void* y, void* c)
{
    int format = frame->format;

    if (handle->stream_info == 0) {
        handle->stream_info = SFXINF_GetStmInf(frame, "COMPO");
    }
    switch (format) {
    case 3:
        SFX_CnvFrmYcc420plnToY84C44(handle, frame, y, c);
        break;
    default:
        SFXLIB_Error(
            handle, (void*)frame,
            "E201193: SFX_CnvFrmY84C44 : frmfmt is not support.");
        break;
    }
}

const int gap_04_80317ABC_rodata = 0;
