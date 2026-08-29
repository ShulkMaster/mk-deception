#include "rw/rwengine.h"
#include "rw/rtanim.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

RtAnimInterpolatorInfo RtAnimInterpolatorInfoBlock[16];
RwFreeList RtAnimAnimationFreeListSpace;

static int _rtAnimAnimationFreeListBlockSize = 0x80;
static int _rtAnimAnimationFreeListPreallocBlocks = 1;

RwFreeList* RtAnimAnimationFreeList;
int RtAnimInterpolatorInfoBlockNumEntries;

static void* AnimOpen(void* instance, int offset, int size) {
    RtAnimAnimationFreeList = RwFreeListCreateAndPreallocateSpace(
        sizeof(RtAnimAnimation), _rtAnimAnimationFreeListBlockSize, 4,
        _rtAnimAnimationFreeListPreallocBlocks,
        &RtAnimAnimationFreeListSpace, 0x4001B);
    if (RtAnimAnimationFreeList == 0) {
        instance = 0;
    }
    return instance;
}

static void* AnimClose(void* instance, int offset, int size) {
    RtAnimInterpolatorInfoBlockNumEntries = 0;
    if (RtAnimAnimationFreeList != 0) {
        RwFreeListDestroy(RtAnimAnimationFreeList);
        RtAnimAnimationFreeList = 0;
    }
    return instance;
}

int RtAnimInitialize(void) {
    int result;

    result = RwEngineRegisterPlugin(0, 0x1B7, AnimOpen, AnimClose) > 0;
    return result;
}

int RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo* info) {
    int index;

    if (RtAnimInterpolatorInfoBlockNumEntries < 16) {
        for (index = 0; index < RtAnimInterpolatorInfoBlockNumEntries;
             index++) {
            if (info->typeID == RtAnimInterpolatorInfoBlock[index].typeID) {
                RwError duplicateError;
                duplicateError.pluginID = 0x1B7;
                duplicateError.errorCode = _rwerror(0);
                RwErrorSet(&duplicateError);
                return 0;
            }
        }

        RtAnimInterpolatorInfoBlock[RtAnimInterpolatorInfoBlockNumEntries] =
            *info;
        RtAnimInterpolatorInfoBlockNumEntries++;
        return 1;
    }

    {
        RwError capacityError;
        capacityError.pluginID = 0x1B7;
        capacityError.errorCode = _rwerror(1);
        RwErrorSet(&capacityError);
    }
    return 0;
}

RtAnimInterpolator* RtAnimInterpolatorCreate(
    int numNodes, int maxInterpKeyFrameSize) {
    void* memory;
    RtAnimInterpolator* interpolator;

    memory = RwEngineInstance->fpMalloc(
        numNodes * maxInterpKeyFrameSize + sizeof(RtAnimInterpolator), 0x3001B);
    interpolator = memory;
    interpolator->numNodes = numNodes;
    interpolator->pCurrentAnim = 0;
    interpolator->pNextFrame = 0;
    interpolator->currentTime = 0.0f;
    interpolator->pAnimCallBack = 0;
    interpolator->animCallBackTime = -1.0f;
    interpolator->pAnimCallBackData = 0;
    interpolator->pAnimLoopCallBack = 0;
    interpolator->pAnimLoopCallBackData = 0;
    interpolator->currentInterpKeyFrameSize = maxInterpKeyFrameSize;
    interpolator->currentAnimKeyFrameSize = -1;
    interpolator->maxInterpKeyFrameSize = maxInterpKeyFrameSize;
    interpolator->isSubInterpolator = 0;
    interpolator->offsetInParent = 0;
    interpolator->parentAnimation = interpolator;
    interpolator->keyFrameApplyCB = 0;
    interpolator->keyFrameInterpolateCB = 0;
    interpolator->keyFrameBlendCB = 0;
    interpolator->keyFrameAddCB = 0;
    return interpolator;
}

void RtAnimInterpolatorDestroy(RtAnimInterpolator* interpolator) {
    RwEngineInstance->fpFree(interpolator);
}
