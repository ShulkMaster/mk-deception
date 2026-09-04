#include "rw/rpskin.h"
#include "runtime/asm_sequences.inc"

/* Blend each position through two weighted skin matrices. */
asm void _rwDlSkinUpdate2WeightsP(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionData* data)
{
    SEQ__rwDlSkinUpdate2WeightsP();
}

/* Blend each position and normal through two weighted skin matrices. */
asm void _rwDlSkinUpdate2WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data)
{
    SEQ__rwDlSkinUpdate2WeightsPN();
}

/* Blend each position through three weighted skin matrices. */
asm void _rwDlSkinUpdate3WeightsP(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionData* data)
{
    SEQ__rwDlSkinUpdate3WeightsP();
}

/* Blend each position and normal through three weighted skin matrices. */
asm void _rwDlSkinUpdate3WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data)
{
    SEQ__rwDlSkinUpdate3WeightsPN();
}

/* Blend each position through four weighted skin matrices. */
asm void _rwDlSkinUpdate4WeightsP(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionData* data)
{
    SEQ__rwDlSkinUpdate4WeightsP();
}

/* Blend each position and normal through four weighted skin matrices. */
asm void _rwDlSkinUpdate4WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data)
{
    SEQ__rwDlSkinUpdate4WeightsPN();
}
