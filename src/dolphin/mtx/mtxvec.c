#include "dolphin/mtx.h"
#include "math/gxVect.h"

/*
 * Soft ceiling: the retail Dolphin SDK implementation is a no-frame
 * paired-single assembly leaf. MWCC 1.2.5n exposes no C intrinsic for its
 * psq/ps_sum sequence, so retain the typed scalar algorithm rather than embed
 * assembly or force registers.
 */
void PSMTXMultVec(const Mtx matrix, const Vec* source, Vec* destination)
{
    Vec result;

    result.x = matrix[0][3] +
               (matrix[0][2] * source->z +
                (matrix[0][0] * source->x + matrix[0][1] * source->y));
    result.y = matrix[1][3] +
               (matrix[1][2] * source->z +
                (matrix[1][0] * source->x + matrix[1][1] * source->y));
    result.z = matrix[2][3] +
               (matrix[2][2] * source->z +
                (matrix[2][0] * source->x + matrix[2][1] * source->y));

    destination->x = result.x;
    destination->y = result.y;
    destination->z = result.z;
}
