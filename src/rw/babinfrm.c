#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwframe.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

typedef struct RwFrameChunkInfo {
    RwV3d right;
    RwV3d up;
    RwV3d at;
    RwV3d pos;
    int parentIndex;
    unsigned int data;
} RwFrameChunkInfo;

extern RwPluginRegistry frameTKList;
extern RwFrame* RwFrameCreate(void);
extern int RwFrameDestroyHierarchy(RwFrame*);
extern RwFrame* RwFrameAddChild(RwFrame*, RwFrame*);
extern RwFrame* RwFrameAddChildNoUpdate(RwFrame*, RwFrame*);
extern RwFrame* RwFrameGetRoot(const RwFrame*);
extern float _rwMatrixNormalError(const RwMatrix*);
extern float _rwMatrixOrthogonalError(const RwMatrix*);
extern float _rwMatrixDeterminant(const RwMatrix*);

static int _rwFrameListDirtyListUpdate = 1;

int RwFrameRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    int offset;

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
    int numFrames;
    unsigned int length;
    unsigned int version;
    int i;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        if (RwStreamRead(stream, &numFrames, sizeof(numFrames)) !=
            sizeof(numFrames)) {
            return 0;
        }
        RwMemNative32(&numFrames, sizeof(numFrames));
        frameList->numFrames = numFrames;
        frameList->frames = RwEngineInstance->fpMalloc(
            numFrames * sizeof(RwFrame*), 0x3000E);
        if (frameList->frames == 0) {
            RwError error;

            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013,
                                       numFrames * sizeof(RwFrame*));
            RwErrorSet(&error);
            return 0;
        }

        for (i = 0; i < numFrames; i++) {
            RwFrameChunkInfo chunk;
            RwFrame* frame;
            RwMatrix* matrix;

            if (RwStreamRead(stream, &chunk, sizeof(chunk)) != sizeof(chunk)) {
                RwEngineInstance->fpFree(frameList->frames);
                return 0;
            }
            RwMemNative32(&chunk, sizeof(chunk));
            frame = RwFrameCreate();
            if (frame == 0) {
                RwEngineInstance->fpFree(frameList->frames);
                return 0;
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
                if (_rwFrameListDirtyListUpdate != 0) {
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
        return 0;
    }

    for (i = 0; i < numFrames; i++) {
        RwFrame* frame = frameList->frames[i];

        if (_rwPluginRegistryReadDataChunks(&frameTKList, stream, frame) ==
            0) {
            RwFrameDestroyHierarchy(frameList->frames[0]);
            RwEngineInstance->fpFree(frameList->frames);
            return 0;
        }
        if (frame == RwFrameGetRoot(frame) &&
            _rwFrameListDirtyListUpdate == 1) {
            RwFrameUpdateObjects(frame);
        }
    }
    return frameList;
}
