#include "rw/rtquat.h"

static inline void multiply_affine_values(RwMatrix* result,
                                          const RwMatrix* left,
                                          const RwMatrix* right)
{
    result->right.x = left->right.x * right->right.x +
                      left->right.y * right->up.x +
                      left->right.z * right->at.x;
    result->right.y = left->right.x * right->right.y +
                      left->right.y * right->up.y +
                      left->right.z * right->at.y;
    result->right.z = left->right.x * right->right.z +
                      left->right.y * right->up.z +
                      left->right.z * right->at.z;

    result->up.x = left->up.x * right->right.x +
                   left->up.y * right->up.x + left->up.z * right->at.x;
    result->up.y = left->up.x * right->right.y +
                   left->up.y * right->up.y + left->up.z * right->at.y;
    result->up.z = left->up.x * right->right.z +
                   left->up.y * right->up.z + left->up.z * right->at.z;

    result->at.x = left->at.x * right->right.x +
                   left->at.y * right->up.x + left->at.z * right->at.x;
    result->at.y = left->at.x * right->right.y +
                   left->at.y * right->up.y + left->at.z * right->at.y;
    result->at.z = left->at.x * right->right.z +
                   left->at.y * right->up.z + left->at.z * right->at.z;

    result->pos.x = right->pos.x + left->pos.x * right->right.x +
                    left->pos.y * right->up.x + left->pos.z * right->at.x;
    result->pos.y = right->pos.y + left->pos.x * right->right.y +
                    left->pos.y * right->up.y + left->pos.z * right->at.y;
    result->pos.z = right->pos.z + left->pos.x * right->right.z +
                    left->pos.y * right->up.z + left->pos.z * right->at.z;
}

void _rpSkinMatrixBlendUpdateASM(RwMatrix* destination,
                                 const RwMatrix* skinToBone,
                                 const RwMatrix* hierarchyMatrices,
                                 const RwMatrix* transform,
                                 const unsigned char* usedBoneList,
                                 unsigned int numUsedBones)
{
    RwMatrix transformValues;

    transformValues.right = transform->right;
    transformValues.up = transform->up;
    transformValues.at = transform->at;
    transformValues.pos = transform->pos;

    do {
        unsigned int bone = *usedBoneList++;
        RwMatrix hierarchyTransform;

        multiply_affine_values(&hierarchyTransform, &hierarchyMatrices[bone],
                               &transformValues);
        multiply_affine_values(&destination[bone], &skinToBone[bone],
                               &hierarchyTransform);
    } while (--numUsedBones != 0);
}
