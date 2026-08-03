#ifndef MOVIE_MWSFX_H
#define MOVIE_MWSFX_H

typedef struct MwsFrameInfo {
    void* frame; /* +0x00 */
} MwsFrameInfo;

typedef struct SfxFrameInfo {
    unsigned char field_0x00[0x7C];
} SfxFrameInfo;

#ifdef __cplusplus
extern "C" {
#endif

void mwPlyFxCnvFrmARGB8888(int handle, MwsFrameInfo* frame, void* output);
void mwPlyFxCnvFrmY84C44(int handle, MwsFrameInfo* frame, void* y, void* c);
void mwPlyFxSetOutBufPitchHeight(int handle, int pitch, int height);
int MWSFD_IsEnableHndl(int handle);
void MWSFSVM_Error(const char* message, ...);
int MWSFSFX_GetSfxHn(int handle);
void MWSFSFX_CnvFrmInfToSfx(int handle, MwsFrameInfo* frame,
                            SfxFrameInfo* output);
void SFX_CnvFrmARGB8888(int handle, SfxFrameInfo* frame_info, void* output);
void SFX_CnvFrmY84C44(int handle, SfxFrameInfo* frame_info, void* y, void* c);

#ifdef __cplusplus
}
#endif

#endif
