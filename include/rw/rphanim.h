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
    int nodeID;
    int nodeIndex;
    int flags;
    RwFrame *pFrame;
} RpHAnimNodeInfo;

typedef struct RpHAnimHierarchy {
    int flags;
    int numNodes;
    RwMatrix *pMatrixArray;
    void *pMatrixArrayUnaligned;
    RpHAnimNodeInfo *pNodeInfo;
    RwFrame *parentFrame;
    struct RpHAnimHierarchy *parentHierarchy;
    int rootParentOffset;
    RtAnimInterpolator *currentAnim;
} RpHAnimHierarchy;

typedef struct RpHAnimKeyFrame {
    struct RpHAnimKeyFrame *prevFrame;
    float time;
    Quat q;
    RwV3d t;
} RpHAnimKeyFrame;

void RpHAnimKeyFrameApply(void *matrix, void *frame);
void RpHAnimKeyFrameInterpolate(void *out, void *in1, void *in2, float time,
                                void *customData);
void RpHAnimKeyFrameBlend(void *out, void *in1, void *in2, float alpha);
RtAnimAnimation *RpHAnimKeyFrameStreamRead(RwStream *, RtAnimAnimation *);
int RpHAnimKeyFrameStreamWrite(RtAnimAnimation *, RwStream *);
int RpHAnimKeyFrameStreamGetSize(RtAnimAnimation *);
void RpHAnimKeyFrameMulRecip(void *frame, void *start);
void RpHAnimKeyFrameAdd(void *out, void *in1, void *in2);

int RpHAnimPluginAttach(void);
RpHAnimHierarchy *RpHAnimHierarchyCreate(int numNodes, unsigned int *nodeFlags,
                                         int *nodeIDs,
                                         RpHAnimHierarchyFlag flags,
                                         int maxInterpKeyFrameSize);
RpHAnimHierarchy *RpHAnimHierarchyDestroy(RpHAnimHierarchy *hierarchy);
RpHAnimHierarchy *RpHAnimFrameGetHierarchy(RwFrame *frame);

#endif
