#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rwcore_types.h"
#include "rw/rwfreelist.h"
#include "rw/rphanim.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"

typedef struct RpHAnimFrameExtension {
    RwInt32 nodeID;
    RpHAnimHierarchy* hierarchy;
} RpHAnimFrameExtension;

typedef struct RpHAnimGlobals {
    RwInt32 frameExtensionOffset;
    RwFreeList* hierarchyFreeList;
} RpHAnimGlobals;

static RwFreeList _rpHAnimHierarchyFreeList;
static RwInt32 _rpHAnimHierarchyFreeListBlockSize = 0x80;
static RwInt32 _rpHAnimHierarchyFreeListPreallocBlocks = 1;
RpHAnimGlobals RpHAnimAtomicGlobals;

static RpHAnimFrameExtension* HAnimFrameExtension(const void* frame)
{
    return (RpHAnimFrameExtension*)((RwUInt8*)frame +
                                    RpHAnimAtomicGlobals.frameExtensionOffset);
}

extern RwInt32 RwFrameRegisterPlugin(RwInt32, RwUInt32,
                                     RwPluginObjectConstructor,
                                     RwPluginObjectDestructor,
                                     RwPluginObjectCopy);
extern RwInt32 RwFrameRegisterPluginStream(
    RwUInt32, RwPluginDataChunkReadCallBack,
    RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack);

static void* HAnimOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RtAnimInterpolatorInfo interpInfo;

    RpHAnimAtomicGlobals.hierarchyFreeList =
        RwFreeListCreateAndPreallocateSpace(
            sizeof(RpHAnimHierarchy), _rpHAnimHierarchyFreeListBlockSize, 4,
            _rpHAnimHierarchyFreeListPreallocBlocks,
            &_rpHAnimHierarchyFreeList, 0x4011E);
    if (RpHAnimAtomicGlobals.hierarchyFreeList == 0) {
        instance = 0;
    }
    interpInfo.typeID = 1;
    interpInfo.interpKeyFrameSize = 0x24;
    interpInfo.animKeyFrameSize = 0x24;
    interpInfo.keyFrameApplyCB = RpHAnimKeyFrameApply;
    interpInfo.keyFrameBlendCB = RpHAnimKeyFrameBlend;
    interpInfo.keyFrameInterpolateCB = RpHAnimKeyFrameInterpolate;
    interpInfo.keyFrameAddCB = RpHAnimKeyFrameAdd;
    interpInfo.keyFrameMulRecipCB = RpHAnimKeyFrameMulRecip;
    interpInfo.keyFrameStreamReadCB = RpHAnimKeyFrameStreamRead;
    interpInfo.keyFrameStreamWriteCB = RpHAnimKeyFrameStreamWrite;
    interpInfo.keyFrameStreamGetSizeCB = RpHAnimKeyFrameStreamGetSize;
    interpInfo.customDataSize = 0;
    RtAnimRegisterInterpolationScheme(&interpInfo);
    return instance;
}

static void* HAnimClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RpHAnimAtomicGlobals.hierarchyFreeList != 0) {
        RwFreeListDestroy(RpHAnimAtomicGlobals.hierarchyFreeList);
        RpHAnimAtomicGlobals.hierarchyFreeList = 0;
    }
    return instance;
}

static void* HAnimConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExtension = HAnimFrameExtension(object);
    frameExtension->hierarchy = 0;
    frameExtension->nodeID = -1;
    return object;
}

static void* HAnimDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExtension = HAnimFrameExtension(object);

    if (frameExtension->hierarchy != 0) {
        RpHAnimHierarchy* hierarchy = frameExtension->hierarchy;
        RwInt32 i;



        for (i = 0; i < hierarchy->numNodes; i++) {
            hierarchy->pNodeInfo[i].pFrame = 0;
        }
        if (hierarchy->parentFrame == object) {
            RpHAnimHierarchyDestroy(hierarchy);
        }
        frameExtension->hierarchy = 0;
    }
    frameExtension->nodeID = -1;
    return object;
}

static void* HAnimCopy(void* dstObject, const void* srcObject,
                       RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* src = HAnimFrameExtension(srcObject);
    RpHAnimFrameExtension* dst = HAnimFrameExtension(dstObject);
    dst->nodeID = src->nodeID;
    if (src->hierarchy != 0) {
        RpHAnimHierarchy* srcHierarchy = src->hierarchy;
        if (!(srcHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
            RpHAnimHierarchy* dstHierarchy = RpHAnimHierarchyCreate(
                srcHierarchy->numNodes, 0, 0,
                srcHierarchy->flags,
                srcHierarchy->currentAnim->maxInterpKeyFrameSize);
            RwInt32 i;
            for (i = 0; i < dstHierarchy->numNodes; i++) {
                dstHierarchy->pNodeInfo[i].pFrame = 0;
                dstHierarchy->pNodeInfo[i].flags = srcHierarchy->pNodeInfo[i].flags;
                dstHierarchy->pNodeInfo[i].nodeIndex = srcHierarchy->pNodeInfo[i].nodeIndex;
                dstHierarchy->pNodeInfo[i].nodeID = srcHierarchy->pNodeInfo[i].nodeID;
            }
            dst->hierarchy = dstHierarchy;
            dstHierarchy->parentFrame = dstObject;
        }
    }
    return dstObject;
}

static RwStream* HAnimWrite(RwStream* stream, RwInt32 binaryLength,
                            const void* object, RwInt32 offset, RwInt32 size)
{
    RwInt32 version = 0x100;
    RwInt32 zero;
    const RpHAnimFrameExtension* frameExtension;
    RpHAnimHierarchy* hierarchy;

    if (!RwStreamWriteInt32(stream, &version, 4)) return 0;
    frameExtension = HAnimFrameExtension(object);
    if (!RwStreamWriteInt32(stream, &frameExtension->nodeID, 4)) return 0;
    hierarchy = frameExtension->hierarchy;
    if (hierarchy != 0 && !(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        RpHAnimNodeInfo* node;
        RwInt32 i;
        if (!RwStreamWriteInt32(stream, &hierarchy->numNodes, 4)) return 0;
        if (!RwStreamWriteInt32(stream, &hierarchy->flags, 4)) return 0;
        if (!RwStreamWriteInt32(stream, &hierarchy->currentAnim->maxInterpKeyFrameSize, 4)) return 0;
        node = hierarchy->pNodeInfo;
        i = 0;
        while (i < hierarchy->numNodes) {
            if (!RwStreamWriteInt32(stream, &node->nodeID, 4)) return 0;
            if (!RwStreamWriteInt32(stream, &node->nodeIndex, 4)) return 0;
            if (!RwStreamWriteInt32(stream, &node->flags, 4)) return 0;
            node++;
            i++;
        }
    } else {
        zero = 0;
        if (!RwStreamWriteInt32(stream, &zero, 4)) return 0;
    }
    return stream;
}

static RwStream* HAnimRead(RwStream* stream, RwInt32 binaryLength,
                           void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExtension = HAnimFrameExtension(object);
    RwInt32 numNodes;
    RwInt32 version;
    RwInt32 flags;
    RwInt32 maxInterpKeyFrameSize;

    if (!RwStreamReadInt32(stream, &version, 4)) return 0;
    if (version != 0x100) return 0;
    if (!RwStreamReadInt32(stream, &frameExtension->nodeID, 4)) return 0;
    if (!RwStreamReadInt32(stream, &numNodes, 4)) return 0;
    if (numNodes > 0) {
        RpHAnimHierarchy* hierarchy;
        void* matrixArrayUnaligned;
        RpHAnimNodeInfo* node;
        RwInt32 i;
        if (!RwStreamReadInt32(stream, &flags, 4)) return 0;
        if (!RwStreamReadInt32(stream, &maxInterpKeyFrameSize, 4)) return 0;
        hierarchy = RwEngineInstance->fpFreeListAlloc(
            RpHAnimAtomicGlobals.hierarchyFreeList, 0x3011E);
        memset(hierarchy, 0, sizeof(*hierarchy));
        if (hierarchy != 0) {
            matrixArrayUnaligned = 0;
            hierarchy->currentAnim = RtAnimInterpolatorCreate(
                numNodes, maxInterpKeyFrameSize);
            hierarchy->flags = flags;
            hierarchy->parentFrame = object;
            hierarchy->numNodes = numNodes;
            hierarchy->parentHierarchy = hierarchy;
            if (hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) {
                hierarchy->pMatrixArray = 0;
                hierarchy->pMatrixArrayUnaligned = 0;
            } else {
                matrixArrayUnaligned = RwEngineInstance->fpMalloc(
                    numNodes * sizeof(RwMatrix) + 15, 0x3011E);
                hierarchy->pMatrixArray = (RwMatrix*)
                    (((RwUInt32)matrixArrayUnaligned + 15) & ~15);
                hierarchy->pMatrixArrayUnaligned = matrixArrayUnaligned;
            }
            hierarchy->pNodeInfo = RwEngineInstance->fpMalloc(
                numNodes * sizeof(RpHAnimNodeInfo), 0x3011E);
            node = hierarchy->pNodeInfo;
            i = 0;
            while (i < hierarchy->numNodes) {
                if (!RwStreamReadInt32(stream, &node->nodeID, 4)) return 0;
                if (!RwStreamReadInt32(stream, &node->nodeIndex, 4)) return 0;
                if (!RwStreamReadInt32(stream, &node->flags, 4)) return 0;
                node->pFrame = 0;
                node++;
                i++;
            }
        }
        frameExtension->hierarchy = hierarchy;
    }
    return stream;
}

static RwInt32 HAnimSize(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpHAnimFrameExtension* frameExtension = HAnimFrameExtension(object);
    RwBool streamData = 1;
    if (frameExtension->nodeID == -1 && frameExtension->hierarchy == 0) {
        streamData = 0;
    }





    if (streamData) {
        RwInt32 streamSize = 4;
        streamSize += 4;
        streamSize += 4;
        if (frameExtension->hierarchy != 0 &&
            !(frameExtension->hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
            streamSize += 4;
            streamSize += 4;
            streamSize += frameExtension->hierarchy->numNodes * 12;
        }
        return streamSize;
    }
    return 0;
}

RwBool RpHAnimPluginAttach(void)
{
    RwInt32 streamOffset;

    RwBool result = 0;
    if (RwEngineRegisterPlugin(0, 0x11E, HAnimOpen, HAnimClose) < 0) {
        return 0;
    }
    RpHAnimAtomicGlobals.frameExtensionOffset = RwFrameRegisterPlugin(
        sizeof(RpHAnimFrameExtension), 0x11E,
        HAnimConstructor, HAnimDestructor, HAnimCopy);
    streamOffset = RwFrameRegisterPluginStream(
        0x11E, HAnimRead, HAnimWrite, HAnimSize);
    if (streamOffset >= 0 && RpHAnimAtomicGlobals.frameExtensionOffset >= 0) {
        result = 1;
    }
    return result;
}

RpHAnimHierarchy* RpHAnimHierarchyCreate(
    RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs,
    RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize)
{
    void* memory;

    RpHAnimHierarchy* hierarchy = RwEngineInstance->fpFreeListAlloc(
        RpHAnimAtomicGlobals.hierarchyFreeList, 0x3011E);
    RwInt32 i;
    hierarchy->currentAnim = RtAnimInterpolatorCreate(
        numNodes, maxInterpKeyFrameSize);
    hierarchy->flags = flags;
    hierarchy->numNodes = numNodes;
    hierarchy->parentFrame = 0;
    if (!(flags & rpHANIMHIERARCHYNOMATRICES)) {
        memory = RwEngineInstance->fpMalloc(
            numNodes * sizeof(RwMatrix) + 15, 0x3011E);
        hierarchy->pMatrixArray = (RwMatrix*)(((RwUInt32)memory + 15) & ~15);
        hierarchy->pMatrixArrayUnaligned = memory;
    } else {
        hierarchy->pMatrixArray = 0;
        hierarchy->pMatrixArrayUnaligned = 0;
    }
    hierarchy->pNodeInfo = RwEngineInstance->fpMalloc(
        numNodes * sizeof(RpHAnimNodeInfo), 0x3011E);
    for (i = 0; i < numNodes; i++) {
        hierarchy->pNodeInfo[i].pFrame = 0;
        if (nodeIDs != 0) hierarchy->pNodeInfo[i].nodeID = nodeIDs[i];
        hierarchy->pNodeInfo[i].nodeIndex = i;
        if (nodeFlags != 0) hierarchy->pNodeInfo[i].flags = nodeFlags[i];
    }
    hierarchy->parentHierarchy = hierarchy;
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy)
{
    RwFrame* parentFrame = hierarchy->parentFrame;
    if (!(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        if (!(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)) {
            RwEngineInstance->fpFree(hierarchy->pMatrixArrayUnaligned);
        }
        RwEngineInstance->fpFree(hierarchy->pNodeInfo);
    }
    hierarchy->pMatrixArrayUnaligned = 0;
    hierarchy->pMatrixArray = 0;
    hierarchy->pNodeInfo = 0;
    RtAnimInterpolatorDestroy(hierarchy->currentAnim);
    RwEngineInstance->fpFreeListFree(
        RpHAnimAtomicGlobals.hierarchyFreeList, hierarchy);
    hierarchy = 0;
    if (parentFrame != 0) {

        RpHAnimFrameExtension* frameExtension = HAnimFrameExtension(parentFrame);
        frameExtension->hierarchy = 0;
    }
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame)
{
    RpHAnimHierarchy* hierarchy = 0;
    RpHAnimFrameExtension* frameExtension = HAnimFrameExtension(frame);
    hierarchy = frameExtension->hierarchy;
    return hierarchy;
}
