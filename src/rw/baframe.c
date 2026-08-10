#include "libmkparticle/rw_engine.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rwtypehf.h"

enum {
    rwFRAMEHIERARCHYSYNCHRONIZED = 0x01,
    rwFRAMEPRIVATEHIERARCHYSYNCHRONIZED = 0x02,
    rwFRAMELTMDIRTY = 0x04,
    rwFRAMEOBJECTSYNCDIRTY = 0x08
};

extern void _rwFrameSyncHierarchyLTM(RwFrame*);

RwPluginRegistry frameTKList = { sizeof(RwFrame), sizeof(RwFrame), 0, 0, 0, 0 };
static RwFreeList frameFreeList;
static RwInt32 _rwFrameFreeListBlockSize = 0x32;
static RwInt32 _rwFrameFreeListPreallocBlocks = 1;
static RwModuleInfo frameModule;

static void rwSetHierarchyRoot(RwFrame* frame, RwFrame* root);
static void FrameDestroyRecurseDeInitLeaf(RwFrame* frame);
static void rwFrameDestroyRecurseDestroyLeaf(RwFrame* frame);
static void rwFrameDestroyRecurse(RwFrame* frame);

static RwFreeList** frameFreeListSlot(void)
{
    return (RwFreeList**)((RwUInt8*)RwEngineInstance +
                          frameModule.globalsOffset);
}

void* _rwFrameOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    frameModule.globalsOffset = offset;
    *frameFreeListSlot() =
        RwFreeListCreateAndPreallocateSpace(
            frameTKList.sizeOfStruct, _rwFrameFreeListBlockSize, 4,
            _rwFrameFreeListPreallocBlocks, &frameFreeList, 0x4000E);
    if (*frameFreeListSlot() == 0)
        return 0;
    rwLinkListInitialize(&RwEngineInstance->dirtyFrameList);
    frameModule.numInstances++;
    return instance;
}

void* _rwFrameClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (*frameFreeListSlot() != 0) {
        RwFreeListDestroy(*frameFreeListSlot());
        *frameFreeListSlot() = 0;
    }
    frameModule.numInstances--;
    return instance;
}

static void rwSetHierarchyRoot(RwFrame* frame, RwFrame* root)
{
    frame->root = root;
    frame = frame->child;
    while (frame != 0) {
        rwSetHierarchyRoot(frame, root);
        frame = frame->next;
    }
}

RwBool RwFrameDirty(const RwFrame* frame)
{
    RwBool dirty;
    frame = frame->root;
    dirty = frame->object.privateFlags & 3;
    return dirty;
}

void _rwFrameInit(RwFrame* frame)
{
    rwObjectInitialize(frame, 0, 0);
    rwLinkListInitialize(&frame->objectList);
    frame->modelling.flags = 3;
    frame->modelling.right.x = frame->modelling.up.y =
        frame->modelling.at.z = 1.0f;
    frame->modelling.right.y = frame->modelling.right.z =
        frame->modelling.up.x = 0.0f;
    frame->modelling.up.z = frame->modelling.at.x =
        frame->modelling.at.y = 0.0f;
    frame->modelling.pos.x = frame->modelling.pos.y =
        frame->modelling.pos.z = 0.0f;
    frame->modelling.flags |= 0x20003;
    frame->ltm.flags = 3;
    frame->ltm.right.x = frame->ltm.up.y = frame->ltm.at.z = 1.0f;
    frame->ltm.right.y = frame->ltm.right.z = frame->ltm.up.x = 0.0f;
    frame->ltm.up.z = frame->ltm.at.x = frame->ltm.at.y = 0.0f;
    frame->ltm.pos.x = frame->ltm.pos.y = frame->ltm.pos.z = 0.0f;
    frame->ltm.flags |= 0x20003;
    frame->child = 0;
    frame->next = 0;
    frame->root = frame;
    _rwPluginRegistryInitObject(&frameTKList, frame);
}

RwFrame* RwFrameCreate(void)
{
    RwFrame* frame = RwEngineInstance->fpFreeListAlloc(
        *frameFreeListSlot(),
        0x3000E);
    if (frame == 0)
        return 0;
    _rwFrameInit(frame);
    return frame;
}

RwFrame* RwFrameRemoveChild(RwFrame* child);

void _rwFrameDeInit(RwFrame* frame)
{

    RwFrame* child;
    _rwPluginRegistryDeInitObject(&frameTKList, frame);
    if (frame->object.parent != 0)
        RwFrameRemoveChild(frame);
    if (frame->object.privateFlags & 3)
        rwLinkListRemoveLLLink(&frame->inDirtyListLink);
    child = frame->child;
    while (child != 0) {
        child->object.parent = 0;
        child = child->next;
    }
}

RwBool RwFrameDestroy(RwFrame* frame)
{
    _rwFrameDeInit(frame);
    RwEngineInstance->fpFreeListFree(
        *frameFreeListSlot(), frame);
    return 1;
}

static void FrameDestroyRecurseDeInitLeaf(RwFrame* frame)
{
    _rwPluginRegistryDeInitObject(&frameTKList, frame);
    if (frame->object.privateFlags & 3)
        rwLinkListRemoveLLLink(&frame->inDirtyListLink);
}

static void rwFrameDestroyRecurseDestroyLeaf(RwFrame* frame)
{
    FrameDestroyRecurseDeInitLeaf(frame);
    RwEngineInstance->fpFreeListFree(
        *frameFreeListSlot(), frame);
}

static void rwFrameDestroyRecurse(RwFrame* frame)
{

    if (frame != 0) {
        RwFrame* child = frame->child;
        while (child != 0) {
            RwFrame* next = child->next;
            rwFrameDestroyRecurse(child);
            child = next;
        }
        rwFrameDestroyRecurseDestroyLeaf(frame);
    }
}

RwBool RwFrameDestroyHierarchy(RwFrame* frame)
{
    rwFrameDestroyRecurse(frame);
    return 1;
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame)
{
    RwUInt32 privateFlags = frame->root->object.privateFlags;
    if (!(privateFlags & 3))
        rwLinkListAddLLLink(&RwEngineInstance->dirtyFrameList,
                            &frame->root->inDirtyListLink);
    frame->root->object.privateFlags = privateFlags | 3;
    frame->object.privateFlags |= rwFRAMELTMDIRTY | rwFRAMEOBJECTSYNCDIRTY;
    return frame;
}

RwMatrix* RwFrameGetLTM(RwFrame* frame)
{
    if (frame->root->object.privateFlags & rwFRAMEHIERARCHYSYNCHRONIZED)
        _rwFrameSyncHierarchyLTM(frame->root);
    return &frame->ltm;
}

RwFrame* RwFrameGetRoot(const RwFrame* frame)
{
    return frame->root;
}

RwFrame* RwFrameAddChildNoUpdate(RwFrame* parent, RwFrame* child)
{

    if (child->object.parent != 0)
        RwFrameRemoveChild(child);
    child->next = parent->child;
    parent->child = child;
    child->object.parent = parent;
    rwSetHierarchyRoot(child, parent->root);
    return parent;
}

RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child)
{

    if (child->object.parent != 0)
        RwFrameRemoveChild(child);
    child->next = parent->child;
    parent->child = child;
    child->object.parent = parent;
    rwSetHierarchyRoot(child, parent->root);
    if (child->object.privateFlags & 3) {
        rwLinkListRemoveLLLink(&child->inDirtyListLink);
        child->object.privateFlags &= ~3;
    }
    RwFrameUpdateObjects(child);
    return parent;
}

RwFrame* RwFrameRemoveChild(RwFrame* child)
{
    RwFrame* previous = ((RwFrame*)child->object.parent)->child;
    if (previous == child) {
        ((RwFrame*)child->object.parent)->child = child->next;
    } else {
        while (previous->next != child)
            previous = previous->next;
        previous->next = child->next;
    }
    child->object.parent = 0;
    child->next = 0;
    rwSetHierarchyRoot(child, child);
    RwFrameUpdateObjects(child);
    return child;
}

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callback, void* data)
{

    RwFrame* child = frame->child;
    while (child != 0) {
        RwFrame* next = child->next;
        if (callback(child, data) == 0)
            return frame;
        child = next;
    }
    return frame;
}

RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* translation, RwInt32 combineOp)
{
    RwMatrixTranslate(&frame->modelling, translation, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameScale(RwFrame* frame, const RwV3d* scale, RwInt32 combineOp)
{
    RwMatrixScale(&frame->modelling, scale, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* matrix, RwInt32 combineOp)
{
    RwMatrixTransform(&frame->modelling, matrix, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle,
                       RwInt32 combineOp)
{
    RwMatrixRotate(&frame->modelling, axis, angle, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameSetIdentity(RwFrame* frame)
{
    frame->modelling.right.x = frame->modelling.up.y =
        frame->modelling.at.z = 1.0f;
    frame->modelling.right.y = frame->modelling.right.z =
        frame->modelling.up.x = 0.0f;
    frame->modelling.up.z = frame->modelling.at.x =
        frame->modelling.at.y = 0.0f;
    frame->modelling.pos.x = frame->modelling.pos.y =
        frame->modelling.pos.z = 0.0f;
    frame->modelling.flags |= 0x20003;
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameOrthoNormalize(RwFrame* frame)
{
    RwMatrixOrthoNormalize(&frame->modelling, &frame->modelling);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callback, void* data)
{
    RwLLLink* next;
    RwLLLink* link = frame->objectList.link.next;
    RwLLLink* sentinel = &frame->objectList.link;
    while (link != sentinel) {

        RwObjectHasFrame* object;
        next = link->next;
        object = (RwObjectHasFrame*)((RwUInt8*)link - 8);
        if (callback((RwObject*)object, data) == 0)
            return frame;
        link = next;
    }
    return frame;
}

RwInt32 RwFrameRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB)
{
    RwInt32 offset = _rwPluginRegistryAddPlugin(
        &frameTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}
