#include "rw/rtquat.h"

extern RwMatrix* RwMatrixMultiply(RwMatrix* destination,
                                  const RwMatrix* first,
                                  const RwMatrix* second);

void _rpSkinMatrixBlendUpdateASM(RwMatrix* destination,
                                 const RwMatrix* skinToBone,
                                 const RwMatrix* hierarchyMatrices,
                                 const RwMatrix* transform,
                                 const RwUInt8* usedBoneList,
                                 RwUInt32 numUsedBones)
{




    RwUInt32 i;

    for (i = 0; i < numUsedBones; i++) {
        RwUInt32 bone = usedBoneList[i];
        RwUInt32 flags = destination[bone].flags;
        RwMatrix hierarchyTransform;

        hierarchyTransform.flags = 0;
        RwMatrixMultiply(&hierarchyTransform, &hierarchyMatrices[bone],
                         transform);
        RwMatrixMultiply(&destination[bone], &skinToBone[bone],
                         &hierarchyTransform);
        destination[bone].flags = flags;
    }
}
