#include "rw/rtquat.h"

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
