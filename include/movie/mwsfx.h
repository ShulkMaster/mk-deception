#ifndef MOVIE_MWSFX_H
#define MOVIE_MWSFX_H

#include "sofdec/sfx.h"

typedef struct MwsFrameInfo {
    void* frame; /* +0x00 */
} MwsFrameInfo;

typedef SFXFrameInfo SfxFrameInfo;

#ifdef __cplusplus
extern "C" {
#endif

void mwPlyFxCnvFrmARGB8888(int handle, MwsFrameInfo* frame, void* output);
void mwPlyFxCnvFrmY84C44(int handle, MwsFrameInfo* frame, void* y, void* c);
void mwPlyFxSetOutBufPitchHeight(int handle, int pitch, int height);
int MWSFD_IsEnableHndl(int handle);
void MWSFSVM_Error(const char* message, ...);
SFXHandle* MWSFSFX_GetSfxHn(int handle);
void MWSFSFX_CnvFrmInfToSfx(int handle, MwsFrameInfo* frame,
                            SfxFrameInfo* output);
void SFX_CnvFrmARGB8888(SFXHandle* handle,
                        const SfxFrameInfo* frame_info, void* output);
void SFX_CnvFrmY84C44(SFXHandle* handle,
                      const SfxFrameInfo* frame_info, void* y, void* c);

#ifdef __cplusplus
}
#endif

#endif
