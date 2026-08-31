#include "sofdec/mpv_mc.h"

int MPV_GetLinkFlg(MPVContext* handle, int* first, int* second)
{
    if (MPVLIB_CheckHn(handle) != 0) {
        return MPVERR_SetCode(0, 0xFF03020E);
    }
    *first = handle->link_flag_0;
    *second = handle->link_flag_1;
    return 0;
}

int MPV_GetVbvBufSiz(MPVContext* handle, int* buffer_size,
                     int* delay, int* byte_rate)
{
    int bit_rate;

    if (MPVLIB_CheckHn(handle) != 0) {
        return MPVERR_SetCode(0, 0xFF03020F);
    }
    *buffer_size = handle->vbv_buffer_units << 11;
    *delay = handle->vbv_delay;
    bit_rate = handle->bit_rate;
    if (bit_rate == 0x3FFFF) {
        *byte_rate = -1;
    } else {
        *byte_rate = handle->vbv_delay * bit_rate / 1800;
    }
    return 0;
}

int MPV_GetBitRate(MPVContext* handle, int* bit_rate)
{
    if (MPVLIB_CheckHn(handle) != 0) {
        return MPVERR_SetCode(0, 0xFF03020D);
    }
    *bit_rate = handle->bit_rate;
    return 0;
}

int MPV_GetPicAtr(MPVContext* handle, MPVPictureInfo* picture)
{
    MPVPictureInfo* current;

    if (MPVLIB_CheckHn(handle) != 0) {
        return MPVERR_SetCode(0, 0xFF03020C);
    }
    current = (MPVPictureInfo*)&handle->condition_state.decoder.picture;
    *picture = *current;
    return 0;
}
