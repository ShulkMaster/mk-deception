#include "rw/rwtypehf.h"

extern RwFrame* RwFrameUpdateObjects(RwFrame* frame);

void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame) {
    RwObjectHasFrame* objectHasFrame = object;

    if (objectHasFrame->object.parent != 0) {
        rwLinkListRemoveLLLink(&objectHasFrame->lFrame);
    }

    ((RwObject*)object)->parent = frame;
    if (frame != 0) {
        rwLinkListAddLLLink(&frame->objectList, &objectHasFrame->lFrame);
        RwFrameUpdateObjects(frame);
    }
}

void _rwObjectHasFrameReleaseFrame(void* object) {
    RwObjectHasFrame* objectHasFrame = object;

    if (objectHasFrame->object.parent != 0) {
        rwLinkListRemoveLLLink(&objectHasFrame->lFrame);
    }
}
