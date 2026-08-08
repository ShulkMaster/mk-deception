#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwstream_internal.h"

typedef struct RwFrameChunkInfo {
    RwV3d right;
    RwV3d up;
    RwV3d at;
    RwV3d pos;
    RwInt32 parentIndex;
    RwUInt32 data;
} RwFrameChunkInfo;

extern RwPluginRegistry frameTKList;
extern RwBool RwStreamFindChunk(RwStream*, RwUInt32, RwUInt32*, RwUInt32*);
extern RwUInt32 RwStreamRead(RwStream*, void*, RwUInt32);
extern void RwMemNative32(void*, RwUInt32);
extern RwFrame* RwFrameCreate(void);
extern RwBool RwFrameDestroyHierarchy(RwFrame*);
extern RwFrame* RwFrameAddChild(RwFrame*, RwFrame*);
extern RwFrame* RwFrameAddChildNoUpdate(RwFrame*, RwFrame*);
extern RwFrame* RwFrameGetRoot(const RwFrame*);
extern RwFrame* RwFrameUpdateObjects(RwFrame*);
extern RwReal _rwMatrixNormalError(const RwMatrix*);
extern RwReal _rwMatrixOrthogonalError(const RwMatrix*);
extern RwReal _rwMatrixDeterminant(const RwMatrix*);

static RwBool _rwFrameListDirtyListUpdate = TRUE;

RwInt32 RwFrameRegisterPluginStream(
    RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    RwInt32 offset;

    offset = _rwPluginRegistryAddPluginStream(&frameTKList, pluginID, readCB,
                                               writeCB, getSizeCB);
    return offset;
}

RwFrameList* _rwFrameListDeinitialize(RwFrameList* frameList) {
    if (frameList->numFrames != 0) {
        RwEngineInstance->fpFree(frameList->frames);
    }
    return frameList;
}

RwFrameList* _rwFrameListStreamRead(RwStream* stream, RwFrameList* frameList) {
    RwInt32 numFrames;
    RwUInt32 length;
    RwUInt32 version;
    RwInt32 i;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return NULL;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        if (RwStreamRead(stream, &numFrames, sizeof(numFrames)) !=
            sizeof(numFrames)) {
            return NULL;
        }
        RwMemNative32(&numFrames, sizeof(numFrames));
        frameList->numFrames = numFrames;
        frameList->frames = RwEngineInstance->fpMalloc(
            numFrames * sizeof(RwFrame*), 0x3000E);
        if (frameList->frames == NULL) {
            RwError error;

            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013,
                                       numFrames * sizeof(RwFrame*));
            RwErrorSet(&error);
            return NULL;
        }

        for (i = 0; i < numFrames; i++) {
            RwFrameChunkInfo chunk;
            RwFrame* frame;
            RwMatrix* matrix;

            if (RwStreamRead(stream, &chunk, sizeof(chunk)) != sizeof(chunk)) {
                RwEngineInstance->fpFree(frameList->frames);
                return NULL;
            }
            RwMemNative32(&chunk, sizeof(chunk));
            frame = RwFrameCreate();
            if (frame == NULL) {
                RwEngineInstance->fpFree(frameList->frames);
                return NULL;
            }
            matrix = &frame->modelling;
            matrix->right = chunk.right;
            matrix->up = chunk.up;
            matrix->at = chunk.at;
            matrix->pos = chunk.pos;
            if (0.01f >= _rwMatrixNormalError(matrix) &&
                0.01f >= _rwMatrixOrthogonalError(matrix) &&
                0.99f <= _rwMatrixDeterminant(matrix)) {
                matrix->flags &= 0xFFFDFFFF;
            } else {
                matrix->flags &= 0xFFFDFFFC;
            }
            frameList->frames[i] = frame;
            if (chunk.parentIndex >= 0) {
                if (_rwFrameListDirtyListUpdate != FALSE) {
                    RwFrameAddChild(frameList->frames[chunk.parentIndex], frame);
                } else {
                    RwFrameAddChildNoUpdate(frameList->frames[chunk.parentIndex],
                                            frame);
                }
            }
        }

    } else {
        RwError error;

        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }

    for (i = 0; i < numFrames; i++) {
        RwFrame* frame = frameList->frames[i];

        if (_rwPluginRegistryReadDataChunks(&frameTKList, stream, frame) ==
            NULL) {
            RwFrameDestroyHierarchy(frameList->frames[0]);
            RwEngineInstance->fpFree(frameList->frames);
            return NULL;
        }
        if (frame == RwFrameGetRoot(frame) &&
            _rwFrameListDirtyListUpdate == TRUE) {
            RwFrameUpdateObjects(frame);
        }
    }
    return frameList;
}
