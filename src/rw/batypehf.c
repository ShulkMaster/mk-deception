#include "rw/rwtypehf.h"
#include "rw/rwframe.h"

void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame) {
    RwObjectHasFrame* objectHasFrame = object;

    if (objectHasFrame->object.parent != 0) {
        objectHasFrame->lFrame.prev->next = objectHasFrame->lFrame.next;
        {
            RwLLLink* previous = objectHasFrame->lFrame.prev;
            objectHasFrame->lFrame.next->prev = previous;
        }
    }

    ((RwObject*)object)->parent = frame;
    if (frame != 0) {
        objectHasFrame->lFrame.next = frame->objectList.link.next;
        objectHasFrame->lFrame.prev = &frame->objectList.link;
        frame->objectList.link.next->prev = &objectHasFrame->lFrame;
        {
            RwLLLink* link = &objectHasFrame->lFrame;
            frame->objectList.link.next = link;
        }
        RwFrameUpdateObjects(frame);
    }
}

void _rwObjectHasFrameReleaseFrame(void* object) {
    RwObjectHasFrame* objectHasFrame = object;

    if (objectHasFrame->object.parent != 0) {
        objectHasFrame->lFrame.prev->next = objectHasFrame->lFrame.next;
        {
            RwLLLink* previous = objectHasFrame->lFrame.prev;
            objectHasFrame->lFrame.next->prev = previous;
        }
    }
}
