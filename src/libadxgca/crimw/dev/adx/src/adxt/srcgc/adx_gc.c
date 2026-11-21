#include "cri/adxt_internal.h"

extern void ADXRNA_SetAdjsfreqFlg(AXRNAHandle* rna, int enabled);

void ADXGC_SetAdjsfreqFlg(ADXTHandle* handle, int enabled)
{
    ADXRNA_SetAdjsfreqFlg(handle->rna, enabled);
}
