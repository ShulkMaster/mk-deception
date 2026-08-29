#include "rw/rwengine.h"
#include "rw/rwfreelist.h"
#include "rw/rwframe.h"
#include "rw/rwframe_internal.h"
#include "rw/rwplcore.h"
#include "rw/rwtypehf.h"

enum {
    rwFRAMEHIERARCHYSYNCHRONIZED = 0x01,
    rwFRAMEPRIVATEHIERARCHYSYNCHRONIZED = 0x02,
    rwFRAMELTMDIRTY = 0x04,
    rwFRAMEOBJECTSYNCDIRTY = 0x08
};

RwPluginRegistry frameTKList = { sizeof(RwFrame), sizeof(RwFrame), 0, 0, 0, 0 };
static RwFreeList frameFreeList;
static int _rwFrameFreeListBlockSize = 0x32;
static int _rwFrameFreeListPreallocBlocks = 1;
static RwModuleInfo frameModule;

static void rwSetHierarchyRoot(RwFrame* frame, RwFrame* root);
static void FrameDestroyRecurseDeInitLeaf(RwFrame* frame);
static void rwFrameDestroyRecurseDestroyLeaf(RwFrame* frame);
static void rwFrameDestroyRecurse(RwFrame* frame);

void* _rwFrameOpen(void* instance, int offset, int size)
{
    RwLLLink* dirtyLink;

    frameModule.globalsOffset = offset;
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                    frameModule.globalsOffset) =
        RwFreeListCreateAndPreallocateSpace(
            frameTKList.sizeOfStruct, _rwFrameFreeListBlockSize, 4,
            _rwFrameFreeListPreallocBlocks, &frameFreeList, 0x4000E);
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        frameModule.globalsOffset) == 0)
        return 0;
    RwEngineInstance->dirtyFrameList.link.next =
        &RwEngineInstance->dirtyFrameList.link;
    dirtyLink = &RwEngineInstance->dirtyFrameList.link;
    RwEngineInstance->dirtyFrameList.link.prev = dirtyLink;
    frameModule.numInstances++;
    return instance;
}

void* _rwFrameClose(void* instance, int offset, int size)
{
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        frameModule.globalsOffset) != 0) {
        RwFreeListDestroy(
            *(RwFreeList**)((unsigned char*)RwEngineInstance +
                            frameModule.globalsOffset));
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        frameModule.globalsOffset) = 0;
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

int RwFrameDirty(const RwFrame* frame)
{
    int dirty;
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
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                       frameModule.globalsOffset),
        0x3000E);
    if (frame == 0)
        return 0;
    _rwFrameInit(frame);
    return frame;
}

RwFrame* RwFrameRemoveChild(RwFrame* child);

void _rwFrameDeInit(RwFrame* frame)
{
    RwFrame* parent;
    RwFrame* child;

    _rwPluginRegistryDeInitObject(&frameTKList, frame);
    parent = frame->object.parent;
    if (parent != 0)
        RwFrameRemoveChild(frame);
    if (frame->object.privateFlags & 3) {
        frame->inDirtyListLink.prev->next = frame->inDirtyListLink.next;
        {
            RwLLLink* previous = frame->inDirtyListLink.prev;
            frame->inDirtyListLink.next->prev = previous;
        }
    }
    child = frame->child;
    while (child != 0) {
        child->object.parent = 0;
        child = child->next;
    }
}

int RwFrameDestroy(RwFrame* frame)
{
    _rwFrameDeInit(frame);
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                       frameModule.globalsOffset),
        frame);
    return 1;
}

static void FrameDestroyRecurseDeInitLeaf(RwFrame* frame)
{
    _rwPluginRegistryDeInitObject(&frameTKList, frame);
    if (frame->object.privateFlags & 3) {
        frame->inDirtyListLink.prev->next = frame->inDirtyListLink.next;
        {
            RwLLLink* previous = frame->inDirtyListLink.prev;
            frame->inDirtyListLink.next->prev = previous;
        }
    }
}

static void rwFrameDestroyRecurseDestroyLeaf(RwFrame* frame)
{
    FrameDestroyRecurseDeInitLeaf(frame);
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                       frameModule.globalsOffset),
        frame);
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

int RwFrameDestroyHierarchy(RwFrame* frame)
{
    rwFrameDestroyRecurse(frame);
    return 1;
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame)
{
    unsigned int privateFlags = frame->root->object.privateFlags;
    if (!(privateFlags & 3)) {
        frame->root->inDirtyListLink.next =
            RwEngineInstance->dirtyFrameList.link.next;
        frame->root->inDirtyListLink.prev =
            &RwEngineInstance->dirtyFrameList.link;
        RwEngineInstance->dirtyFrameList.link.next->prev =
            &frame->root->inDirtyListLink;
        {
            RwLLLink* link = &frame->root->inDirtyListLink;
            RwEngineInstance->dirtyFrameList.link.next = link;
        }
    }
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
        child->inDirtyListLink.prev->next = child->inDirtyListLink.next;
        child->inDirtyListLink.next->prev = child->inDirtyListLink.prev;
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

RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* translation, int combineOp)
{
    RwMatrixTranslate(&frame->modelling, translation, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameScale(RwFrame* frame, const RwV3d* scale, int combineOp)
{
    RwMatrixScale(&frame->modelling, scale, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* matrix, int combineOp)
{
    RwMatrixTransform(&frame->modelling, matrix, combineOp);
    RwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, float angle,
                       int combineOp)
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
        object = (RwObjectHasFrame*)((unsigned char*)link - 8);
        if (callback((RwObject*)object, data) == 0)
            return frame;
        link = next;
    }
    return frame;
}

int RwFrameRegisterPlugin(int size, unsigned int pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB)
{
    int offset = _rwPluginRegistryAddPlugin(
        &frameTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}
