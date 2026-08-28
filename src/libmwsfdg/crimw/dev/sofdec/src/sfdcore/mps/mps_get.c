#include "cri/mps.h"

int MPS_GetPketHd(MpsHandle* handle, MpsPacketHeader* out) {
    if (MPSLIB_CheckHn(handle) != 0) {
        return MPSLIB_SetErr(0, 0xFF020203);
    }
    *out = handle->payload.headers.packet_header;
    return 0;
}

int MPS_GetLastSysHd(MpsHandle* handle, MpsSystemHeader* out) {
    if (MPSLIB_CheckHn(handle) != 0) {
        return MPSLIB_SetErr(0, 0xFF020202);
    }
    *out = handle->payload.headers.last_system_header;
    return 0;
}

int MPS_GetSysHd(MpsHandle* handle, MpsSystemHeader* out, int index) {
    if (MPSLIB_CheckHn(handle) != 0) {
        return MPSLIB_SetErr(0, 0xFF020202);
    }
    *out = handle->payload.headers.system_headers[index];
    return 0;
}

int MPS_GetPackHd(MpsHandle* handle, MpsPackHeader* out) {
    if (MPSLIB_CheckHn(handle) != 0) {
        return MPSLIB_SetErr(0, 0xFF020201);
    }
    *out = handle->payload.headers.pack_header;
    return 0;
}

void MPSGET_Finish(void) {
}

void MPSGET_Init(void) {
}
