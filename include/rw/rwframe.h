#ifndef RW_RWFRAME_H
#define RW_RWFRAME_H

#include "rw/rwcore_types.h"

#ifdef __cplusplus
extern "C" {
#endif

RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwFrame* RwFrameCreate(void);
int RwFrameDestroy(RwFrame* frame);
int RwFrameDestroyHierarchy(RwFrame* frame);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
RwFrame* RwFrameGetRoot(const RwFrame* frame);
RwFrame* RwFrameAddChildNoUpdate(RwFrame* parent, RwFrame* child);
RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child);
RwFrame* RwFrameRemoveChild(RwFrame* child);
RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callback,
                               void* data);
RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* translation,
                          int combineOp);
RwFrame* RwFrameScale(RwFrame* frame, const RwV3d* scale, int combineOp);
RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* matrix,
                          int combineOp);
RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, float angle,
                       int combineOp);
RwFrame* RwFrameSetIdentity(RwFrame* frame);
RwFrame* RwFrameOrthoNormalize(RwFrame* frame);
RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callback,
                              void* data);
int RwFrameRegisterPlugin(
    int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int RwFrameRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);
void* _rwFrameOpen(void* instance, int offset, int size);
void* _rwFrameClose(void* instance, int offset, int size);

#ifdef __cplusplus
}
#endif

#endif
