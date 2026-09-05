#include "rw/rwengine.h"
#include "rw/rwframe_internal.h"
#include "rw/rwtypehf.h"
#include "rw/rwvector.h"

/* TODO: [near miss] 99.583336%; flags/object GPR homes; scope check unchanged. */
static void FrameSyncHierarchyRecurse(RwFrame* frame, unsigned int inheritedFlags) {
    while (frame != 0) {
        unsigned int flags = inheritedFlags | frame->object.privateFlags;

        if ((int)(flags & 0x04) != 0) {
            if ((frame->object.privateFlags & 0x20) != 0) {
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
                    RwObjectHasFrame* object =
                        RW_CONTAINER_OF(link, RwObjectHasFrame, lFrame);
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
        frame->object.privateFlags &= ~0x08;
        FrameSyncHierarchyRecurseNoLTM(frame->child);
        frame = frame->next;
    }
}

static void FrameSyncHierarchy(RwFrame* root) {
    unsigned int flags = root->object.privateFlags;

    if ((int)(flags & 0x01) != 0) {
        if ((int)(flags & 0x04) != 0) {
            root->ltm = root->modelling;
        }
        if (root->objectList.link.next != &root->objectList.link) {
            RwLLLink* link = root->objectList.link.next;
            RwLLLink* sentinel = &root->objectList.link;
            while (link != sentinel) {
                RwObjectHasFrame* object = (RwObjectHasFrame*)
                    ((unsigned char*)link - 8);
                object->sync(object);
                link = link->next;
            }
        }
        FrameSyncHierarchyRecurse(root->child,
                                  flags & 0x04);
    } else {
        if (root->objectList.link.next != &root->objectList.link) {
            RwLLLink* link = root->objectList.link.next;
            RwLLLink* sentinel = &root->objectList.link;
            while (link != sentinel) {
                RwObjectHasFrame* object = (RwObjectHasFrame*)
                    ((unsigned char*)link - 8);
                object->sync(object);
                link = link->next;
            }
        }
        FrameSyncHierarchyRecurseNoLTM(root->child);
    }
    root->object.privateFlags = flags & ~0x0FU;
}

int _rwFrameSyncDirty(void) {
    RwLLLink* link = RwEngineInstance->dirtyFrameList.link.next;
    RwLLLink* sentinel = &RwEngineInstance->dirtyFrameList.link;

    while (link != sentinel) {
        RwFrame* frame = (RwFrame*)((unsigned char*)link - 8);
        FrameSyncHierarchy(frame);
        link = link->next;
    }
    RwEngineInstance->dirtyFrameList.link.next =
        &RwEngineInstance->dirtyFrameList.link;
    {
        RwLLLink* previousSentinel = &RwEngineInstance->dirtyFrameList.link;
        RwEngineInstance->dirtyFrameList.link.prev = previousSentinel;
    }
    return 1;
}

static void FrameSyncHierarchyLTMRecurse(RwFrame* frame,
                                         unsigned int inheritedFlags) {
    while (frame != 0) {
        unsigned int flags = inheritedFlags | frame->object.privateFlags;

        if ((int)(flags & 0x04) != 0) {
            if ((frame->object.privateFlags & 0x20) != 0) {
                frame->ltm = frame->modelling;
                RwV3dTransformPoints(&frame->ltm.pos, &frame->modelling.pos,
                                     1, &((RwFrame*)frame->object.parent)->ltm);
                RwMatrixUpdate(&frame->ltm);
            } else {
                RwMatrixMultiply(&frame->ltm, &frame->modelling,
                                 &((RwFrame*)frame->object.parent)->ltm);
            }
            frame->object.privateFlags &= ~0x04;
        }
        FrameSyncHierarchyLTMRecurse(frame->child, flags);
        frame = frame->next;
    }
}

void _rwFrameSyncHierarchyLTM(RwFrame* root) {
    unsigned int flags = root->object.privateFlags;

    if ((int)(flags & 0x04) != 0) {
        root->ltm = root->modelling;
    }
    FrameSyncHierarchyLTMRecurse(root->child, flags);
    root->object.privateFlags = flags & ~0x05;
}
