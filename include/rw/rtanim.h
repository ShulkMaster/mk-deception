#ifndef RW_RTANIM_H
#define RW_RTANIM_H

#include "rw/rwplcore.h"

typedef struct RtAnimAnimation RtAnimAnimation;
typedef struct RtAnimInterpolator RtAnimInterpolator;

typedef void (*RtAnimKeyFrameApplyCallBack)(void*, void*);
typedef void (*RtAnimKeyFrameBlendCallBack)(void*, void*, void*, float);
typedef void (*RtAnimKeyFrameInterpolateCallBack)(void*, void*, void*, float,
                                                   void*);
typedef void (*RtAnimKeyFrameAddCallBack)(void*, void*, void*);
typedef void (*RtAnimKeyFrameMulRecipCallBack)(void*, void*);
typedef RtAnimAnimation* (*RtAnimKeyFrameStreamReadCallBack)(RwStream*,
                                                             RtAnimAnimation*);
typedef int (*RtAnimKeyFrameStreamWriteCallBack)(RtAnimAnimation*,
                                                    RwStream*);
typedef int (*RtAnimKeyFrameStreamGetSizeCallBack)(RtAnimAnimation*);
typedef RtAnimInterpolator* (*RtAnimCallBack)(RtAnimInterpolator*, void*);

typedef struct RtAnimInterpolatorInfo {
    int typeID;
    int interpKeyFrameSize;
    int animKeyFrameSize;
    RtAnimKeyFrameApplyCallBack keyFrameApplyCB;
    RtAnimKeyFrameBlendCallBack keyFrameBlendCB;
    RtAnimKeyFrameInterpolateCallBack keyFrameInterpolateCB;
    RtAnimKeyFrameAddCallBack keyFrameAddCB;
    RtAnimKeyFrameMulRecipCallBack keyFrameMulRecipCB;
    RtAnimKeyFrameStreamReadCallBack keyFrameStreamReadCB;
    RtAnimKeyFrameStreamWriteCallBack keyFrameStreamWriteCB;
    RtAnimKeyFrameStreamGetSizeCallBack keyFrameStreamGetSizeCB;
    int customDataSize;
} RtAnimInterpolatorInfo;

struct RtAnimAnimation {
    RtAnimInterpolatorInfo* interpInfo;
    int numFrames;
    int flags;
    float duration;
    void* pFrames;
    void* customData;
};

struct RtAnimInterpolator {
    RtAnimAnimation* pCurrentAnim;
    float currentTime;
    void* pNextFrame;
    RtAnimCallBack pAnimCallBack;
    void* pAnimCallBackData;
    float animCallBackTime;
    RtAnimCallBack pAnimLoopCallBack;
    void* pAnimLoopCallBackData;
    int maxInterpKeyFrameSize;
    int currentInterpKeyFrameSize;
    int currentAnimKeyFrameSize;
    int numNodes;
    int isSubInterpolator;
    int offsetInParent;
    RtAnimInterpolator* parentAnimation;
    RtAnimKeyFrameApplyCallBack keyFrameApplyCB;
    RtAnimKeyFrameBlendCallBack keyFrameBlendCB;
    RtAnimKeyFrameInterpolateCallBack keyFrameInterpolateCB;
    RtAnimKeyFrameAddCallBack keyFrameAddCB;
};

int RtAnimInitialize(void);
int RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo* info);
RtAnimInterpolator* RtAnimInterpolatorCreate(int numNodes,
                                             int maxInterpKeyFrameSize);
void RtAnimInterpolatorDestroy(RtAnimInterpolator* interpolator);

#endif
