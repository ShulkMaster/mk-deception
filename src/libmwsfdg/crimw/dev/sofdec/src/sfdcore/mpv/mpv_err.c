#include "sofdec/mpv_error.h"
#include "sofdec/mpv_mc.h"

extern int MPVLIB_CheckHn(MPVContext* handle);

static MPVErrorInfo mpverrinf;
/* Retail object tail alignment between mpv_err and the next archive member. */
int gap_06_80497F54_bss;

int MPVERR_SetCode(MPVContext* handle, int error)
{
    if (handle == 0) {
        mpverrinf.first_error = error;
        if (error != 0 && mpverrinf.callback != 0) {
            mpverrinf.callback(mpverrinf.callback_object, error);
        }
    } else {
        handle->error_info.first_error = error;
        if (error != 0 && handle->error_info.callback != 0) {
            handle->error_info.callback(handle->error_info.callback_object,
                                        error);
        }
    }

    return error;
}

int MPV_SetErrFunc(MPVContext* handle, MPVErrorCallback callback, int object)
{
    if (MPVLIB_CheckHn(handle) != 0) {
        return MPVERR_SetCode(0, 0xFF030203);
    }

    handle->error_info.callback = callback;
    handle->error_info.callback_object = object;
    return 0;
}

void MPVERR_InitErrInf(MPVErrorInfo* info)
{
    info->callback = 0;
    info->callback_object = 0;
    info->first_error = 0;
    info->field_0C = 0;
    info->field_10 = 0;
}

void MPVERR_Init(void)
{
    mpverrinf.callback = 0;
    mpverrinf.callback_object = 0;
    mpverrinf.first_error = 0;
    mpverrinf.field_0C = 0;
    mpverrinf.field_10 = 0;
}
