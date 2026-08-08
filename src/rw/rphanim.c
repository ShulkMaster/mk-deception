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

#define HANIMFRAMEEXTENSION(frame) \
    ((RpHAnimFrameExtension*)((RwUInt8*)(frame) + \
                              RpHAnimAtomicGlobals.frameExtensionOffset))

extern void RpHAnimKeyFrameApply(void*, void*);
extern void RpHAnimKeyFrameBlend(void*, void*, void*, RwReal);
extern void RpHAnimKeyFrameInterpolate(void*, void*, void*, RwReal, void*);
extern void RpHAnimKeyFrameAdd(void*, void*, void*);
extern void RpHAnimKeyFrameMulRecip(void*, void*);
extern RtAnimAnimation* RpHAnimKeyFrameStreamRead(RwStream*, RtAnimAnimation*);
extern RwBool RpHAnimKeyFrameStreamWrite(RtAnimAnimation*, RwStream*);
extern RwInt32 RpHAnimKeyFrameStreamGetSize(RtAnimAnimation*);
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
    if (RpHAnimAtomicGlobals.hierarchyFreeList == NULL) {
        instance = NULL;
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
    if (RpHAnimAtomicGlobals.hierarchyFreeList != NULL) {
        RwFreeListDestroy(RpHAnimAtomicGlobals.hierarchyFreeList);
        RpHAnimAtomicGlobals.hierarchyFreeList = NULL;
    }
    return instance;
}

static void* HAnimConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExtension = HANIMFRAMEEXTENSION(object);
    frameExtension->hierarchy = NULL;
    frameExtension->nodeID = -1;
    return object;
}

static void* HAnimDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExtension = HANIMFRAMEEXTENSION(object);

    if (frameExtension->hierarchy != NULL) {
        RpHAnimHierarchy* hierarchy = frameExtension->hierarchy;
        RwInt32 i;

        /* Retail repeats this indexed address calculation before the store;
         * clean C intentionally omits the unused first calculation. */
        for (i = 0; i < hierarchy->numNodes; i++) {
            hierarchy->pNodeInfo[i].pFrame = NULL;
        }
        if (hierarchy->parentFrame == object) {
            RpHAnimHierarchyDestroy(hierarchy);
        }
        frameExtension->hierarchy = NULL;
    }
    frameExtension->nodeID = -1;
    return object;
}

static void* HAnimCopy(void* dstObject, const void* srcObject,
                       RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* src = HANIMFRAMEEXTENSION(srcObject);
    RpHAnimFrameExtension* dst = HANIMFRAMEEXTENSION(dstObject);
    dst->nodeID = src->nodeID;
    if (src->hierarchy != NULL) {
        RpHAnimHierarchy* srcHierarchy = src->hierarchy;
        if (!(srcHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
            RpHAnimHierarchy* dstHierarchy = RpHAnimHierarchyCreate(
                srcHierarchy->numNodes, NULL, NULL,
                srcHierarchy->flags,
                srcHierarchy->currentAnim->maxInterpKeyFrameSize);
            RwInt32 i;
            for (i = 0; i < dstHierarchy->numNodes; i++) {
                dstHierarchy->pNodeInfo[i].pFrame = NULL;
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

    if (!RwStreamWriteInt32(stream, &version, 4)) return NULL;
    frameExtension = HANIMFRAMEEXTENSION(object);
    if (!RwStreamWriteInt32(stream, &frameExtension->nodeID, 4)) return NULL;
    hierarchy = frameExtension->hierarchy;
    if (hierarchy != NULL && !(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        RpHAnimNodeInfo* node;
        RwInt32 i;
        if (!RwStreamWriteInt32(stream, &hierarchy->numNodes, 4)) return NULL;
        if (!RwStreamWriteInt32(stream, &hierarchy->flags, 4)) return NULL;
        if (!RwStreamWriteInt32(stream, &hierarchy->currentAnim->maxInterpKeyFrameSize, 4)) return NULL;
        node = hierarchy->pNodeInfo;
        i = 0;
        while (i < hierarchy->numNodes) {
            if (!RwStreamWriteInt32(stream, &node->nodeID, 4)) return NULL;
            if (!RwStreamWriteInt32(stream, &node->nodeIndex, 4)) return NULL;
            if (!RwStreamWriteInt32(stream, &node->flags, 4)) return NULL;
            node++;
            i++;
        }
    } else {
        zero = 0;
        if (!RwStreamWriteInt32(stream, &zero, 4)) return NULL;
    }
    return stream;
}

static RwStream* HAnimRead(RwStream* stream, RwInt32 binaryLength,
                           void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExtension = HANIMFRAMEEXTENSION(object);
    RwInt32 numNodes;
    RwInt32 version;
    RwInt32 flags;
    RwInt32 maxInterpKeyFrameSize;

    if (!RwStreamReadInt32(stream, &version, 4)) return NULL;
    if (version != 0x100) return NULL;
    if (!RwStreamReadInt32(stream, &frameExtension->nodeID, 4)) return NULL;
    if (!RwStreamReadInt32(stream, &numNodes, 4)) return NULL;
    if (numNodes > 0) {
        RpHAnimHierarchy* hierarchy;
        void* matrixArrayUnaligned;
        RpHAnimNodeInfo* node;
        RwInt32 i;
        if (!RwStreamReadInt32(stream, &flags, 4)) return NULL;
        if (!RwStreamReadInt32(stream, &maxInterpKeyFrameSize, 4)) return NULL;
        hierarchy = RwEngineInstance->fpFreeListAlloc(
            RpHAnimAtomicGlobals.hierarchyFreeList, 0x3011E);
        memset(hierarchy, 0, sizeof(*hierarchy));
        if (hierarchy != NULL) {
            matrixArrayUnaligned = NULL;
            hierarchy->currentAnim = RtAnimInterpolatorCreate(
                numNodes, maxInterpKeyFrameSize);
            hierarchy->flags = flags;
            hierarchy->parentFrame = object;
            hierarchy->numNodes = numNodes;
            hierarchy->parentHierarchy = hierarchy;
            if (hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) {
                hierarchy->pMatrixArray = NULL;
                hierarchy->pMatrixArrayUnaligned = NULL;
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
                if (!RwStreamReadInt32(stream, &node->nodeID, 4)) return NULL;
                if (!RwStreamReadInt32(stream, &node->nodeIndex, 4)) return NULL;
                if (!RwStreamReadInt32(stream, &node->flags, 4)) return NULL;
                node->pFrame = NULL;
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
    const RpHAnimFrameExtension* frameExtension = HANIMFRAMEEXTENSION(object);
    RwBool streamData = TRUE;
    if (frameExtension->nodeID == -1 && frameExtension->hierarchy == NULL) {
        streamData = FALSE;
    }
    /*
     * Retail copies streamData through a second nonvolatile register and uses
     * the save/restore helpers; retaining that redundant alias would only
     * force register lifetime and does not change the stream-size algorithm.
     */
    if (streamData) {
        RwInt32 streamSize = 4;
        streamSize += 4;
        streamSize += 4;
        if (frameExtension->hierarchy != NULL &&
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
    /* Retail carries an overwritten pre-registration result initialization. */
    RwBool result = FALSE;
    if (RwEngineRegisterPlugin(0, 0x11E, HAnimOpen, HAnimClose) < 0) {
        return FALSE;
    }
    RpHAnimAtomicGlobals.frameExtensionOffset = RwFrameRegisterPlugin(
        sizeof(RpHAnimFrameExtension), 0x11E,
        HAnimConstructor, HAnimDestructor, HAnimCopy);
    streamOffset = RwFrameRegisterPluginStream(
        0x11E, HAnimRead, HAnimWrite, HAnimSize);
    if (streamOffset >= 0 && RpHAnimAtomicGlobals.frameExtensionOffset >= 0) {
        result = TRUE;
    }
    return result;
}

RpHAnimHierarchy* RpHAnimHierarchyCreate(
    RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs,
    RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize)
{
    void* memory;
    /* Remaining retail difference is numNodes/matrix-memory register coloring. */
    RpHAnimHierarchy* hierarchy = RwEngineInstance->fpFreeListAlloc(
        RpHAnimAtomicGlobals.hierarchyFreeList, 0x3011E);
    RwInt32 i;
    hierarchy->currentAnim = RtAnimInterpolatorCreate(
        numNodes, maxInterpKeyFrameSize);
    hierarchy->flags = flags;
    hierarchy->numNodes = numNodes;
    hierarchy->parentFrame = NULL;
    if (!(flags & rpHANIMHIERARCHYNOMATRICES)) {
        memory = RwEngineInstance->fpMalloc(
            numNodes * sizeof(RwMatrix) + 15, 0x3011E);
        hierarchy->pMatrixArray = (RwMatrix*)(((RwUInt32)memory + 15) & ~15);
        hierarchy->pMatrixArrayUnaligned = memory;
    } else {
        hierarchy->pMatrixArray = NULL;
        hierarchy->pMatrixArrayUnaligned = NULL;
    }
    hierarchy->pNodeInfo = RwEngineInstance->fpMalloc(
        numNodes * sizeof(RpHAnimNodeInfo), 0x3011E);
    for (i = 0; i < numNodes; i++) {
        hierarchy->pNodeInfo[i].pFrame = NULL;
        if (nodeIDs != NULL) hierarchy->pNodeInfo[i].nodeID = nodeIDs[i];
        hierarchy->pNodeInfo[i].nodeIndex = i;
        if (nodeFlags != NULL) hierarchy->pNodeInfo[i].flags = nodeFlags[i];
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
    hierarchy->pMatrixArrayUnaligned = NULL;
    hierarchy->pMatrixArray = NULL;
    hierarchy->pNodeInfo = NULL;
    RtAnimInterpolatorDestroy(hierarchy->currentAnim);
    RwEngineInstance->fpFreeListFree(
        RpHAnimAtomicGlobals.hierarchyFreeList, hierarchy);
    hierarchy = NULL;
    if (parentFrame != NULL) {
        /* Retail materializes this extension; only save-helper selection differs. */
        RpHAnimFrameExtension* frameExtension = HANIMFRAMEEXTENSION(parentFrame);
        frameExtension->hierarchy = NULL;
    }
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame)
{
    RpHAnimHierarchy* hierarchy = NULL;
    RpHAnimFrameExtension* frameExtension = HANIMFRAMEEXTENSION(frame);
    hierarchy = frameExtension->hierarchy;
    return hierarchy;
}
