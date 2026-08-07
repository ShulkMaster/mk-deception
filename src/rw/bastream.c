#include "libmkparticle/rw_engine.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream.h"

extern RwInt32 _rwerror(RwInt32, ...);
extern RwError* RwErrorSet(RwError*);
extern void* memcpy(void*, const void*, RwUInt32);

static RwFreeList _rwStreamFreeList;
static RwInt32 _rwStreamFreeListBlockSize = 0x10;
static RwInt32 _rwStreamFreeListPreallocBlocks = 1;
static RwModuleInfo streamModule;

#define STREAM_ERROR(code) do { RwError error; error.pluginID = 1; \
    error.errorCode = _rwerror(code); RwErrorSet(&error); } while (0)
#define STREAM_ERROR_ARG(code, arg) do { RwError error; error.pluginID = 1; \
    error.errorCode = _rwerror(code, arg); RwErrorSet(&error); } while (0)
#define STREAM_FREE_LIST \
    RWPLUGINOFFSET(RwFreeList*, RwEngineInstance, streamModule.globalsOffset)

/* Near miss: exact module lifecycle; callback parameter/local coloring differs. */
void* _rwStreamModuleOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RwFreeList* list;
    streamModule.globalsOffset = offset;
    list = RwFreeListCreateAndPreallocateSpace(sizeof(RwStream),
        _rwStreamFreeListBlockSize, 4, _rwStreamFreeListPreallocBlocks,
        &_rwStreamFreeList, 0x40404);
    RWPLUGINOFFSET(RwFreeList*, RwEngineInstance, offset) = list;
    if (list == NULL) return NULL;
    ++streamModule.numInstances;
    return instance;
}

void* _rwStreamModuleClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (STREAM_FREE_LIST != NULL) RwFreeListDestroy(STREAM_FREE_LIST);
    --streamModule.numInstances;
    return instance;
}

/* Near miss: exact ftell validation; only callback-result coloring differs. */
static RwStream* StreamFileInitialize(RwStream* stream, void* file)
{
    if (RwEngineInstance->fileFuncs.rwftell(file) == -1) return NULL;
    stream->data.file.file = file;
    return stream;
}

/* Near miss: exact access dispatch/errors; only local lifetime scheduling differs. */
static RwStream* StreamFileNameInitialize(RwStream* stream,
    RwStreamAccessType access, const RwChar* name)
{
    void* file = NULL;
    RwStream* result = NULL;
    switch (access) {
    case rwSTREAMREAD: file = RwEngineInstance->fileFuncs.rwfopen(name, "rb"); break;
    case rwSTREAMWRITE: file = RwEngineInstance->fileFuncs.rwfopen(name, "wb"); break;
    case rwSTREAMAPPEND: file = RwEngineInstance->fileFuncs.rwfopen(name, "ab"); break;
    default: STREAM_ERROR(0xD); break;
    }
    if (file != NULL) { stream->data.file.file = file; result = stream; }
    else STREAM_ERROR_ARG(0x80000002, (void*)name);
    return result;
}

/* Near miss: exact memory-state initialization; switch register coloring differs. */
static RwStream* StreamMemoryInitialize(RwStream* stream,
    RwStreamAccessType access, RwMemory* memory)
{
    RwStream* result = NULL;
    switch (access) {
    case rwSTREAMREAD:
        stream->data.memory.position = 0; stream->data.memory.length = memory->length;
        stream->data.memory.start = memory->start; result = stream; break;
    case rwSTREAMWRITE:
        stream->data.memory.position = 0; stream->data.memory.length = 0;
        stream->data.memory.start = NULL; result = stream; break;
    case rwSTREAMAPPEND:
        stream->data.memory.position = memory->length; stream->data.memory.length = memory->length;
        stream->data.memory.start = memory->start; result = stream; break;
    default: STREAM_ERROR(0xD); break;
    }
    return result;
}

static RwStream* StreamCustomInitialize(RwStream* stream,
    const RwStreamCustom* custom)
{
    memcpy(&stream->data.custom, custom, sizeof(RwStreamCustom));
    return stream;
}

/* Near miss: exact typed dispatcher; one local-result scheduling difference remains. */
RwStream* _rwStreamInitialize(RwStream* stream, RwBool owned, RwStreamType type,
    RwStreamAccessType access, void* data)
{
    RwStream* result = NULL;
    if (stream == NULL) return NULL;
    stream->type = type; stream->accessType = access; stream->owned = owned;
    switch (type) {
    case rwSTREAMFILE: result = StreamFileInitialize(stream, data); break;
    case rwSTREAMFILENAME: result = StreamFileNameInitialize(stream, access, data); break;
    case rwSTREAMMEMORY: result = StreamMemoryInitialize(stream, access, data); break;
    case rwSTREAMCUSTOM: result = StreamCustomInitialize(stream, data); break;
    default: STREAM_ERROR(0xE); break;
    }
    return result;
}

/* Near miss: exact callback/memory/error CFG; stack and register allocation differ. */
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length)
{
    switch (stream->type) {
    case rwSTREAMFILE: case rwSTREAMFILENAME: {
        RwUInt32 count = RwEngineInstance->fileFuncs.rwfread(buffer, 1, length,
            stream->data.file.file);
        if (count != length) {
            if (RwEngineInstance->fileFuncs.rwfeof(stream->data.file.file)) STREAM_ERROR(5);
            else STREAM_ERROR(0x8000001A);
        }
        return count;
    }
    case rwSTREAMMEMORY:
        if (length > stream->data.memory.length - stream->data.memory.position) {
            length = stream->data.memory.length - stream->data.memory.position;
            STREAM_ERROR(5);
        }
        memcpy(buffer, stream->data.memory.start + stream->data.memory.position, length);
        stream->data.memory.position += length; return length;
    case rwSTREAMCUSTOM:
        return stream->data.custom.read(stream->data.custom.data, buffer, length);
    default: STREAM_ERROR(0xE); return 0;
    }
}

/* Near miss: exact growth/hint/callback CFG; local register scheduling differs. */
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length)
{
    switch (stream->type) {
    case rwSTREAMFILE: case rwSTREAMFILENAME:
        if (RwEngineInstance->fileFuncs.rwfwrite(buffer, 1, length,
            stream->data.file.file) != length) { STREAM_ERROR(0x8000001C); return NULL; }
        return stream;
    case rwSTREAMMEMORY:
        if (stream->data.memory.start == NULL) {
            stream->data.memory.start = RwEngineInstance->fpMalloc(0x200, 0x30404);
            if (stream->data.memory.start == NULL) {
                STREAM_ERROR_ARG(0x80000013, 0x200); return NULL;
            }
            stream->data.memory.length = 0x200;
        }
        if (stream->data.memory.length - stream->data.memory.position < length) {
            RwUInt32 oldLength = stream->data.memory.length;
            RwUInt32 newLength = oldLength + (length < 0x200 ? 0x200 : length);
            RwUInt8* start = RwEngineInstance->fpRealloc(stream->data.memory.start,
                newLength, 0x01030404);
            if (start == NULL) {
                STREAM_ERROR_ARG(0x80000013, newLength - oldLength); return NULL;
            }
            stream->data.memory.start = start; stream->data.memory.length = newLength;
        }
        memcpy(stream->data.memory.start + stream->data.memory.position, buffer, length);
        stream->data.memory.position += length; return stream;
    case rwSTREAMCUSTOM:
        return stream->data.custom.write(stream->data.custom.data, buffer, length) ? stream : NULL;
    default: STREAM_ERROR(0xE); return NULL;
    }
}

/* Near miss: exact unsigned clamp and callback CFG; register coloring differs. */
RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset)
{
    if (offset == 0) return stream;
    switch (stream->type) {
    case rwSTREAMFILE: case rwSTREAMFILENAME:
        if (RwEngineInstance->fileFuncs.rwfseek(stream->data.file.file, offset, 1)) {
            if (RwEngineInstance->fileFuncs.rwfeof(stream->data.file.file)) STREAM_ERROR(5);
            return NULL;
        }
        return stream;
    case rwSTREAMMEMORY:
        if (stream->data.memory.position + offset > stream->data.memory.length) {
            stream->data.memory.position = stream->data.memory.length; STREAM_ERROR(5); return NULL;
        }
        stream->data.memory.position += offset; return stream;
    case rwSTREAMCUSTOM:
        return stream->data.custom.skip(stream->data.custom.data, offset) ? stream : NULL;
    default: STREAM_ERROR(0xE); return NULL;
    }
}

/* Near miss: exact ownership/close semantics; result lifetime scheduling differs. */
RwBool RwStreamClose(RwStream* stream, void* data)
{
    RwBool result;
    switch (stream->type) {
    case rwSTREAMFILE: result = TRUE; break;
    case rwSTREAMFILENAME:
        result = RwEngineInstance->fileFuncs.rwfclose(stream->data.file.file) == 0; break;
    case rwSTREAMMEMORY:
        if (stream->accessType != rwSTREAMREAD && data != NULL) {
            ((RwMemory*)data)->start = stream->data.memory.start;
            ((RwMemory*)data)->length = stream->data.memory.position;
        }
        result = TRUE; break;
    case rwSTREAMCUSTOM:
        if (stream->data.custom.close != NULL) stream->data.custom.close(stream->data.custom.data);
        result = TRUE; break;
    default: STREAM_ERROR(0xE); return FALSE;
    }
    if (stream->owned) RwEngineInstance->fpFreeListFree(STREAM_FREE_LIST, stream);
    return result;
}

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType access, void* data)
{
    RwStream* stream = RwEngineInstance->fpFreeListAlloc(STREAM_FREE_LIST, 0x30404);
    if (_rwStreamInitialize(stream, TRUE, type, access, data) == NULL) {
        RwEngineInstance->fpFreeListFree(STREAM_FREE_LIST, stream); stream = NULL;
    }
    return stream;
}
