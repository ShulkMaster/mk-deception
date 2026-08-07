#include "libmkparticle/rw_engine.h"
#include "rw/rwtypehf.h"

RwMatrix* RwMatrixUpdate(RwMatrix* matrix);
RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1,
                           const RwMatrix* matrixIn2);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn,
                            RwInt32 numPoints, const RwMatrix* matrix);

enum {
    rwFRAMEHIERARCHYSYNCHRONIZED = 0x01,
    rwFRAMELTMDIRTY = 0x04,
    rwFRAMEOBJECTSYNCDIRTY = 0x08,
    rwFRAMEMODELLINGIDENTITY = 0x20
};

static void FrameSyncHierarchyRecurse(RwFrame* frame, RwUInt32 inheritedFlags) {
    while (frame != 0) {
        RwUInt32 flags = inheritedFlags | frame->object.privateFlags;

        if ((RwInt32)(flags & rwFRAMELTMDIRTY) != 0) {
            if ((frame->object.privateFlags & rwFRAMEMODELLINGIDENTITY) != 0) {
                frame->ltm = frame->modelling;
                RwV3dTransformPoints(&frame->ltm.pos, &frame->modelling.pos,
                                     1, &((RwFrame*)frame->object.parent)->ltm);
                RwMatrixUpdate(&frame->ltm);
            } else {
                RwMatrixMultiply(&frame->ltm, &frame->modelling,
                                 &((RwFrame*)frame->object.parent)->ltm);
            }
        }
        {
            if (frame->objectList.link.next != &frame->objectList.link) {
                RwLLLink* link = frame->objectList.link.next;
                RwLLLink* sentinel = &frame->objectList.link;
                while (link != sentinel) {
                    RwObjectHasFrame* object = (RwObjectHasFrame*)
                        ((unsigned char*)link - 8);
                    object->sync(object);
                    link = link->next;
                }
            }
        }
        frame->object.privateFlags &= ~0x0C;
        FrameSyncHierarchyRecurse(frame->child, flags);
        frame = frame->next;
    }
}

static void FrameSyncHierarchyRecurseNoLTM(RwFrame* frame) {
    while (frame != 0) {
        if (frame->objectList.link.next != &frame->objectList.link) {
            RwLLLink* link = frame->objectList.link.next;
            RwLLLink* sentinel = &frame->objectList.link;
            while (link != sentinel) {
                RwObjectHasFrame* object = (RwObjectHasFrame*)
                    ((unsigned char*)link - 8);
                object->sync(object);
                link = link->next;
            }
        }
        frame->object.privateFlags &= ~rwFRAMEOBJECTSYNCDIRTY;
        FrameSyncHierarchyRecurseNoLTM(frame->child);
        frame = frame->next;
    }
}

static void FrameSyncHierarchy(RwFrame* root) {
    RwUInt32 flags = root->object.privateFlags;
    RwLLLink* link;

    if ((RwInt32)(flags & rwFRAMEHIERARCHYSYNCHRONIZED) != 0) {
        if ((RwInt32)(flags & rwFRAMELTMDIRTY) != 0) {
            root->ltm = root->modelling;
        }
        if (root->objectList.link.next != &root->objectList.link) {
            RwLLLink* sentinel = &root->objectList.link;
            link = root->objectList.link.next;
            while (link != sentinel) {
                RwObjectHasFrame* object = (RwObjectHasFrame*)
                    ((unsigned char*)link - 8);
                object->sync(object);
                link = link->next;
            }
        }
        FrameSyncHierarchyRecurse(root->child,
                                  flags & rwFRAMELTMDIRTY);
    } else {
        if (root->objectList.link.next != &root->objectList.link) {
            RwLLLink* sentinel = &root->objectList.link;
            link = root->objectList.link.next;
            while (link != sentinel) {
                RwObjectHasFrame* object = (RwObjectHasFrame*)
                    ((unsigned char*)link - 8);
                object->sync(object);
                link = link->next;
            }
        }
        FrameSyncHierarchyRecurseNoLTM(root->child);
    }
    root->object.privateFlags = flags & 0xF0;
}

RwBool _rwFrameSyncDirty(void) {
    RwLLLink* link = RwEngineInstance->dirtyFrameList.link.next;
    RwLLLink* sentinel = &RwEngineInstance->dirtyFrameList.link;

    while (link != sentinel) {
        RwFrame* frame = (RwFrame*)((unsigned char*)link - 8);
        FrameSyncHierarchy(frame);
        link = link->next;
    }
    {
        RwLLLink* nextSentinel = &RwEngineInstance->dirtyFrameList.link;
        RwEngineInstance->dirtyFrameList.link.next = nextSentinel;
    }
    {
        RwLLLink* previousSentinel = &RwEngineInstance->dirtyFrameList.link;
        RwEngineInstance->dirtyFrameList.link.prev = previousSentinel;
    }
    return TRUE;
}

static void FrameSyncHierarchyLTMRecurse(RwFrame* frame,
                                         RwUInt32 inheritedFlags) {
    while (frame != 0) {
        RwUInt32 flags = inheritedFlags | frame->object.privateFlags;

        if ((RwInt32)(flags & rwFRAMELTMDIRTY) != 0) {
            if ((frame->object.privateFlags & rwFRAMEMODELLINGIDENTITY) != 0) {
                frame->ltm = frame->modelling;
                RwV3dTransformPoints(&frame->ltm.pos, &frame->modelling.pos,
                                     1, &((RwFrame*)frame->object.parent)->ltm);
                RwMatrixUpdate(&frame->ltm);
            } else {
                RwMatrixMultiply(&frame->ltm, &frame->modelling,
                                 &((RwFrame*)frame->object.parent)->ltm);
            }
            frame->object.privateFlags &= ~rwFRAMELTMDIRTY;
        }
        FrameSyncHierarchyLTMRecurse(frame->child, flags);
        frame = frame->next;
    }
}

void _rwFrameSyncHierarchyLTM(RwFrame* root) {
    RwUInt32 flags = root->object.privateFlags;

    if ((RwInt32)(flags & rwFRAMELTMDIRTY) != 0) {
        root->ltm = root->modelling;
    }
    FrameSyncHierarchyLTMRecurse(root->child, flags);
    root->object.privateFlags = flags & ~0x05;
}
