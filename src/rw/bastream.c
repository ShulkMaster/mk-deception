#include "rw/rwengine.h"
#include "runtime/cstring.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream.h"

static RwFreeList _rwStreamFreeList;
static int _rwStreamFreeListBlockSize = 0x10;
static int _rwStreamFreeListPreallocBlocks = 1;
static RwModuleInfo streamModule;

void *_rwStreamModuleOpen(void *instance, int offset, int size) {
  streamModule.globalsOffset = offset;
  *(RwFreeList **)((unsigned char *)RwEngineInstance + streamModule.globalsOffset) =
      RwFreeListCreateAndPreallocateSpace(
          sizeof(RwStream), _rwStreamFreeListBlockSize, 4,
          _rwStreamFreeListPreallocBlocks, &_rwStreamFreeList, 0x40404);
  if (*(RwFreeList **)((unsigned char *)RwEngineInstance +
                       streamModule.globalsOffset) == 0)
    return 0;
  ++streamModule.numInstances;
  return instance;
}

void *_rwStreamModuleClose(void *instance, int offset, int size) {
  if (*(RwFreeList **)((unsigned char *)RwEngineInstance +
                       streamModule.globalsOffset) != 0)
    RwFreeListDestroy(*(RwFreeList **)((unsigned char *)RwEngineInstance +
                                       streamModule.globalsOffset));
  --streamModule.numInstances;
  return instance;
}

static RwStream *StreamFileInitialize(RwStream *stream, void *file) {
  if (RwEngineInstance->fileFuncs.tell(file) == -1)
    return 0;
  stream->data.file.file = file;
  return stream;
}

static RwStream *StreamFileNameInitialize(RwStream *stream,
                                          RwStreamAccessType access,
                                          const char *name) {
  RwStream *result = 0;
  void *file = 0;
  switch (access) {
  case 1:
    file = RwEngineInstance->fileFuncs.open(name, "rb");
    break;
  case 2:
    file = RwEngineInstance->fileFuncs.open(name, "wb");
    break;
  case 3:
    file = RwEngineInstance->fileFuncs.open(name, "ab");
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xD);
    RwErrorSet(&error);
  } break;
  }
  if (file != 0) {
    stream->data.file.file = file;
    result = stream;
  } else {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000002, (void *)name);
    RwErrorSet(&error);
  }
  return result;
}

static RwStream *StreamMemoryInitialize(RwStream *stream,
                                        RwStreamAccessType access,
                                        RwMemory *memory) {
  RwStream *result = 0;
  switch (access) {
  case 1:
    stream->data.memory.position = 0;
    stream->data.memory.length = memory->length;
    stream->data.memory.start = memory->start;
    result = stream;
    break;
  case 2:
    stream->data.memory.position = 0;
    stream->data.memory.length = 0;
    stream->data.memory.start = 0;
    result = stream;
    break;
  case 3:
    stream->data.memory.position = memory->length;
    stream->data.memory.length = memory->length;
    stream->data.memory.start = memory->start;
    result = stream;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xD);
    RwErrorSet(&error);
  } break;
  }
  return result;
}

static RwStream *StreamCustomInitialize(RwStream *stream,
                                        const RwStreamCustom *custom) {
  memcpy(&stream->data.custom, custom, sizeof(RwStreamCustom));
  return stream;
}

RwStream *_rwStreamInitialize(RwStream *stream, int owned, RwStreamType type,
                              RwStreamAccessType access, void *data) {
  RwStream *result = 0;
  if (stream == 0)
    return result;
  stream->type = type;
  stream->accessType = access;
  stream->owned = owned;
  switch (type) {
  case 1:
    result = StreamFileInitialize(stream, data);
    break;
  case 2:
    result = StreamFileNameInitialize(stream, access, data);
    break;
  case 3:
    result = StreamMemoryInitialize(stream, access, data);
    break;
  case 4:
    result = StreamCustomInitialize(stream, data);
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xE);
    RwErrorSet(&error);
  } break;
  }
  return result;
}

unsigned int RwStreamRead(RwStream *stream, void *buffer, unsigned int length) {
  void *file;
  unsigned int count;

  switch (stream->type) {
  case 1:
  case 2:
    file = stream->data.file.file;
    count = RwEngineInstance->fileFuncs.read(buffer, 1, length, file);
    if (count != length) {
      if (RwEngineInstance->fileFuncs.eof(file)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(5);
        RwErrorSet(&error);
      } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001A);
        RwErrorSet(&error);
      }
    }
    return count;
  case 3: {
    RwStreamMemory *memory = &stream->data.memory;
    if (length > memory->length - memory->position) {
      length = memory->length - memory->position;
      {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(5);
        RwErrorSet(&error);
      }
    }
    memcpy(buffer, memory->start + memory->position, length);
    memory->position += length;
    return length;
  }
  case 4:
    return stream->data.custom.read(stream->data.custom.data, buffer, length);
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xE);
    RwErrorSet(&error);
  }
    return 0;
  }
}

RwStream *RwStreamWrite(RwStream *stream, const void *buffer, unsigned int length) {
  void *file;
  unsigned int count;

  switch (stream->type) {
  case 1:
  case 2:
    file = stream->data.file.file;
    count = RwEngineInstance->fileFuncs.write(buffer, 1, length, file);
    if (count != length) {
      {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001C);
        RwErrorSet(&error);
      }
      return 0;
    }
    return stream;
  case 3: {
    RwStreamMemory *memory = &stream->data.memory;
    if (memory->start == 0) {
      memory->start = RwEngineInstance->fpMalloc(0x200, 0x30404);
      if (memory->start == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x200);
        RwErrorSet(&error);
        return 0;
      }
      memory->length = 0x200;
    }
    if (memory->length - memory->position < length) {
      unsigned int newLength;
      unsigned char *start;
      if (length < 0x200) {
        newLength = memory->length + 0x200;
      } else {
        newLength = length + memory->length;
      }
      start = RwEngineInstance->fpRealloc(memory->start, newLength,
                                          0x01030404);
      if (start == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode =
            _rwerror(0x80000013, newLength - memory->length);
        RwErrorSet(&error);
        return 0;
      }
      memory->start = start;
      memory->length = newLength;
    }
    memcpy(memory->start + memory->position, buffer, length);
    memory->position += length;
    return stream;
  }
  case 4:
    if (stream->data.custom.write(stream->data.custom.data, buffer, length)) {
      return stream;
    }
    return 0;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xE);
    RwErrorSet(&error);
  }
    return 0;
  }
}

RwStream *RwStreamSkip(RwStream *stream, unsigned int offset) {
  void *file;
  RwStream *result;

  if (offset == 0)
    return stream;
  switch (stream->type) {
  case 1:
  case 2:
    file = stream->data.file.file;
    if (RwEngineInstance->fileFuncs.seek(file, offset, 1)) {
      if (RwEngineInstance->fileFuncs.eof(file)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(5);
        RwErrorSet(&error);
      }
      result = 0;
    } else {
      result = stream;
    }
    return result;
  case 3: {
    RwStreamMemory *memory = &stream->data.memory;
    if (memory->position + offset > memory->length) {
      memory->position = memory->length;
      {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(5);
        RwErrorSet(&error);
      }
      return 0;
    }
    memory->position += offset;
    return stream;
  }
  case 4:
    if (stream->data.custom.skip(stream->data.custom.data, offset)) {
      return stream;
    }
    return 0;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xE);
    RwErrorSet(&error);
  }
    return 0;
  }
}

int RwStreamClose(RwStream *stream, void *data) {
  int result;
  int closeResult;
  switch (stream->type) {
  case 1:
    result = 1;
    break;
  case 2:
    if (RwEngineInstance->fileFuncs.close(stream->data.file.file) == 0) {
      closeResult = 1;
    } else {
      closeResult = 0;
    }
    result = closeResult;
    break;
  case 3: {
    RwMemory *memory = data;
    if (stream->accessType != 1 && data != 0) {
      memory->start = stream->data.memory.start;
      memory->length = stream->data.memory.position;
    }
    result = 1;
    break;
  }
  case 4:
    if (stream->data.custom.close != 0)
      stream->data.custom.close(stream->data.custom.data);
    result = 1;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0xE);
    RwErrorSet(&error);
  }
    return 0;
  }
  if (stream->owned) {
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList **)((unsigned char *)RwEngineInstance +
                         streamModule.globalsOffset),
        stream);
  }
  return result;
}

RwStream *RwStreamOpen(RwStreamType type, RwStreamAccessType access,
                       void *data) {
  RwStream *stream = RwEngineInstance->fpFreeListAlloc(
      *(RwFreeList **)((unsigned char *)RwEngineInstance +
                       streamModule.globalsOffset),
      0x30404);
  if (_rwStreamInitialize(stream, 1, type, access, data) == 0) {
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList **)((unsigned char *)RwEngineInstance +
                         streamModule.globalsOffset),
        stream);
    stream = 0;
  }
  return stream;
}
