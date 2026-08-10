#ifndef RW_RTANIM_H
#define RW_RTANIM_H

#include "rw/rwplcore.h"

typedef struct RtAnimAnimation RtAnimAnimation;
typedef struct RtAnimInterpolator RtAnimInterpolator;

typedef void (*RtAnimKeyFrameApplyCallBack)(void*, void*);
typedef void (*RtAnimKeyFrameBlendCallBack)(void*, void*, void*, RwReal);
typedef void (*RtAnimKeyFrameInterpolateCallBack)(void*, void*, void*, RwReal,
                                                   void*);
typedef void (*RtAnimKeyFrameAddCallBack)(void*, void*, void*);
typedef void (*RtAnimKeyFrameMulRecipCallBack)(void*, void*);
typedef RtAnimAnimation* (*RtAnimKeyFrameStreamReadCallBack)(RwStream*,
                                                             RtAnimAnimation*);
typedef RwBool (*RtAnimKeyFrameStreamWriteCallBack)(RtAnimAnimation*,
                                                    RwStream*);
typedef RwInt32 (*RtAnimKeyFrameStreamGetSizeCallBack)(RtAnimAnimation*);
typedef RtAnimInterpolator* (*RtAnimCallBack)(RtAnimInterpolator*, void*);

typedef struct RtAnimInterpolatorInfo {
    RwInt32 typeID;
    RwInt32 interpKeyFrameSize;
    RwInt32 animKeyFrameSize;
    RtAnimKeyFrameApplyCallBack keyFrameApplyCB;
    RtAnimKeyFrameBlendCallBack keyFrameBlendCB;
    RtAnimKeyFrameInterpolateCallBack keyFrameInterpolateCB;
    RtAnimKeyFrameAddCallBack keyFrameAddCB;
    RtAnimKeyFrameMulRecipCallBack keyFrameMulRecipCB;
    RtAnimKeyFrameStreamReadCallBack keyFrameStreamReadCB;
    RtAnimKeyFrameStreamWriteCallBack keyFrameStreamWriteCB;
    RtAnimKeyFrameStreamGetSizeCallBack keyFrameStreamGetSizeCB;
    RwInt32 customDataSize;
} RtAnimInterpolatorInfo;

struct RtAnimAnimation {
    RtAnimInterpolatorInfo* interpInfo;
    RwInt32 numFrames;
    RwInt32 flags;
    RwReal duration;
    void* pFrames;
    void* customData;
};

struct RtAnimInterpolator {
    RtAnimAnimation* pCurrentAnim;
    RwReal currentTime;
    void* pNextFrame;
    RtAnimCallBack pAnimCallBack;
    void* pAnimCallBackData;
    RwReal animCallBackTime;
    RtAnimCallBack pAnimLoopCallBack;
    void* pAnimLoopCallBackData;
    RwInt32 maxInterpKeyFrameSize;
    RwInt32 currentInterpKeyFrameSize;
    RwInt32 currentAnimKeyFrameSize;
    RwInt32 numNodes;
    RwBool isSubInterpolator;
    RwInt32 offsetInParent;
    RtAnimInterpolator* parentAnimation;
    RtAnimKeyFrameApplyCallBack keyFrameApplyCB;
    RtAnimKeyFrameBlendCallBack keyFrameBlendCB;
    RtAnimKeyFrameInterpolateCallBack keyFrameInterpolateCB;
    RtAnimKeyFrameAddCallBack keyFrameAddCB;
};

RwBool RtAnimInitialize(void);
RwBool RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo* info);
RtAnimInterpolator* RtAnimInterpolatorCreate(RwInt32 numNodes,
                                             RwInt32 maxInterpKeyFrameSize);
void RtAnimInterpolatorDestroy(RtAnimInterpolator* interpolator);

#endif
