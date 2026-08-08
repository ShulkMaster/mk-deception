#ifndef RW_RPHANIM_H
#define RW_RPHANIM_H

#include "rw/rtanim.h"
#include "rw/rwcore_types.h"

typedef enum RpHAnimHierarchyFlag {
    rpHANIMHIERARCHYSUBHIERARCHY = 0x01,
    rpHANIMHIERARCHYNOMATRICES = 0x02,
    rpHANIMHIERARCHYUPDATEMODELLINGMATRICES = 0x1000,
    rpHANIMHIERARCHYUPDATELTMS = 0x2000,
    rpHANIMHIERARCHYLOCALSPACEMATRICES = 0x4000
} RpHAnimHierarchyFlag;

typedef struct RpHAnimNodeInfo {
    RwInt32 nodeID;
    RwInt32 nodeIndex;
    RwInt32 flags;
    RwFrame* pFrame;
} RpHAnimNodeInfo;

typedef struct RpHAnimHierarchy {
    RwInt32 flags;
    RwInt32 numNodes;
    RwMatrix* pMatrixArray;
    void* pMatrixArrayUnaligned;
    RpHAnimNodeInfo* pNodeInfo;
    RwFrame* parentFrame;
    struct RpHAnimHierarchy* parentHierarchy;
    RwInt32 rootParentOffset;
    RtAnimInterpolator* currentAnim;
} RpHAnimHierarchy;

RwBool RpHAnimPluginAttach(void);
RpHAnimHierarchy* RpHAnimHierarchyCreate(
    RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs,
    RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize);
RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy);
RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame);

#endif
