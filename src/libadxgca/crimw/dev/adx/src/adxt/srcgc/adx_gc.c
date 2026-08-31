#include "dolphin/types.h"

typedef struct AdxGcHandle {
    u8 reserved_00[0x0C];
    void* rna;
} AdxGcHandle;

extern void ADXRNA_SetAdjsfreqFlg(void* rna);

void ADXGC_SetAdjsfreqFlg(AdxGcHandle* handle, int enabled)
{
    (void)enabled;
    ADXRNA_SetAdjsfreqFlg(handle->rna);
}
