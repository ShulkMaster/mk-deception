#ifndef RW_RPSKIN_H
#define RW_RPSKIN_H

typedef struct RpAtomic RpAtomic;
typedef struct RpGeometry RpGeometry;
typedef struct RpSkin RpSkin;
typedef struct RwMatrix RwMatrix;

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, void* hierarchy);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, int type);

#endif
