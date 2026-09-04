#include "rw/rtquat.h"

/* TODO: Retail hand-schedules this loop with paired-single loads, fused matrix
 * products, and paired stores. This portable implementation preserves its
 * intent: update only the listed bones with skinToBone * (hierarchy * transform)
 * while retaining each destination matrix's flags. Recheck only if an honest
 * compiler path capable of emitting the paired-single sequence is recovered. */
void _rpSkinMatrixBlendUpdateASM(RwMatrix* destination,
                                 const RwMatrix* skinToBone,
                                 const RwMatrix* hierarchyMatrices,
                                 const RwMatrix* transform,
                                 const unsigned char* usedBoneList,
                                 unsigned int numUsedBones)
{
    unsigned int i;

    for (i = 0; i < numUsedBones; i++) {
        unsigned int bone = usedBoneList[i];
        unsigned int flags = destination[bone].flags;
        RwMatrix hierarchyTransform;

        hierarchyTransform.flags = 0;
        RwMatrixMultiply(&hierarchyTransform, &hierarchyMatrices[bone],
                         transform);
        RwMatrixMultiply(&destination[bone], &skinToBone[bone],
                         &hierarchyTransform);
        destination[bone].flags = flags;
    }
}
