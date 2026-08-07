#include "rw/rwtypehf.h"

extern RwFrame* RwFrameUpdateObjects(RwFrame* frame);

static inline void rwLLLinkRemove(RwLLLink* link) {
    RwLLLink* next = link->next;
    RwLLLink* prev = link->prev;
    prev->next = next;
    next->prev = prev;
}

static inline void rwLinkListAddLLLink(RwLLLink* list, RwLLLink* link) {
    link->next = list->next;
    link->prev = list;
    list->next->prev = link;
    list->next = link;
}

void _rwObjectHasFrameSetFrame(RwObjectHasFrame* object, RwFrame* frame) {
    RwLLLink* frameObjects;

    if (object->object.parent != 0) {
        rwLLLinkRemove(&object->lFrame);
    }

    object->object.parent = frame;
    if (frame != 0) {
        frameObjects = &frame->objectList.link;
        rwLinkListAddLLLink(frameObjects, &object->lFrame);
        RwFrameUpdateObjects(frame);
    }
}

void _rwObjectHasFrameReleaseFrame(RwObjectHasFrame* object) {
    if (object->object.parent != 0) {
        rwLLLinkRemove(&object->lFrame);
    }
}
