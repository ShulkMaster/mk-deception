#include "sofdec/mpv_mc.h"
#include "dolphin/types.h"

extern int MPVLIB_CheckHn(MPVContext* context);
extern int MPVERR_SetCode(MPVContext* context, int error);
extern int MPV_GoNextDelimSj(SJ* stream);
extern int MPV_MoveChunk(SJ* stream, int channel, int size);
extern int MPVHDEC_DecPicture(MPVContext* context, SJ* stream);
extern int MPVM2V_DecodeFrm(MPVContext* context, SJ* stream,
                            MPVFrameBuffers* buffers);
extern void MPVUMC_SetGqr(void);
extern void DCT_FsriSetGqr(void);
extern void MPVUMC_InitOutRfb(MPVContext* context);
extern void MPVCMC_InitMcOiRt(MPVContext* context);
extern void MPVCMC_SetCcnt(MPVContext* context);
extern void MPVCDEC_InitFrm(void* context);
extern void MPVUMC_EndOfFrame(MPVContext* context);
extern void UTY_PushGqr(u32 saved[8]);
extern void UTY_PopGqr(u32 saved[8]);

int MPV_SkipFrmSj(MPVContext* context, SJ* stream)
{
    int delimiter_type;
    int error;

    if (MPVLIB_CheckHn(context) != 0) {
        return MPVERR_SetCode(0, 0xFF03020A);
    }

    error = 0xFF030305;
    for (;;) {
        delimiter_type = MPV_GoNextDelimSj(stream);
        if (delimiter_type == 0) {
            break;
        }
        if ((delimiter_type & 0xCC) != 0) {
            error = 0;
            break;
        }
        if (MPV_MoveChunk(stream, 1, 4) != 4) {
            break;
        }
    }
    return MPVERR_SetCode(context, error);
}

int MPV_DecodeFrmSj(MPVContext* context, SJ* stream,
                    MPVFrameBuffers* buffers)
{
    u32 saved_gqr[8];
    int initial_decoded;
    int initial_skipped;
    int result;

    if (MPVLIB_CheckHn(context) != 0) {
        return MPVERR_SetCode(0, 0xFF030209);
    }

    UTY_PushGqr(saved_gqr);
    MPVUMC_SetGqr();
    DCT_FsriSetGqr();
    if (context->field_358 == 2) {
        return MPVM2V_DecodeFrm(context, stream, buffers);
    }

    initial_decoded = context->error_info.field_0C;
    initial_skipped = context->error_info.field_10;
    context->frame_buffers = *buffers;
    MPVUMC_InitOutRfb(context);
    MPVCMC_InitMcOiRt(context);
    MPVCMC_SetCcnt(context);
    MPVCDEC_InitFrm(context);
    result = MPVHDEC_DecPicture(context, stream);
    MPVUMC_EndOfFrame(context);
    *buffers->picture_info =
        *(MPVPictureInfo*)&context->condition_state.decoder.picture;
    buffers->decoded_dct_count =
        context->error_info.field_0C - initial_decoded;
    buffers->skipped_dct_count =
        context->error_info.field_10 - initial_skipped;
    UTY_PopGqr(saved_gqr);
    return result;
}

void MPVFRM_Init(void)
{
}
