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
    RwFrame *pFrame;
} RpHAnimNodeInfo;

typedef struct RpHAnimHierarchy {
    RwInt32 flags;
    RwInt32 numNodes;
    RwMatrix *pMatrixArray;
    void *pMatrixArrayUnaligned;
    RpHAnimNodeInfo *pNodeInfo;
    RwFrame *parentFrame;
    struct RpHAnimHierarchy *parentHierarchy;
    RwInt32 rootParentOffset;
    RtAnimInterpolator *currentAnim;
} RpHAnimHierarchy;

typedef struct RpHAnimKeyFrame {
    struct RpHAnimKeyFrame *prevFrame;
    RwReal time;
    RtQuat q;
    RwV3d t;
} RpHAnimKeyFrame;

void RpHAnimKeyFrameApply(void *matrix, void *frame);
void RpHAnimKeyFrameInterpolate(void *out, void *in1, void *in2, RwReal time,
                                void *customData);
void RpHAnimKeyFrameBlend(void *out, void *in1, void *in2, RwReal alpha);
RtAnimAnimation *RpHAnimKeyFrameStreamRead(RwStream *, RtAnimAnimation *);
RwBool RpHAnimKeyFrameStreamWrite(RtAnimAnimation *, RwStream *);
RwInt32 RpHAnimKeyFrameStreamGetSize(RtAnimAnimation *);
void RpHAnimKeyFrameMulRecip(void *frame, void *start);
void RpHAnimKeyFrameAdd(void *out, void *in1, void *in2);

RwBool RpHAnimPluginAttach(void);
RpHAnimHierarchy *RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32 *nodeFlags,
                                         RwInt32 *nodeIDs,
                                         RpHAnimHierarchyFlag flags,
                                         RwInt32 maxInterpKeyFrameSize);
RpHAnimHierarchy *RpHAnimHierarchyDestroy(RpHAnimHierarchy *hierarchy);
RpHAnimHierarchy *RpHAnimFrameGetHierarchy(RwFrame *frame);

#endif
