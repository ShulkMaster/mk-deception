#include "cri/adxt_internal.h"
#include "dolphin/types.h"

extern void ADXRNA_SetAdjsfreqFlg(struct AXRNAHandle* rna, int enabled);

void ADXGC_SetAdjsfreqFlg(ADXTHandle* handle, int enabled)
{
    ADXRNA_SetAdjsfreqFlg(handle->rna, enabled);
}
