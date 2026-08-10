#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rwerror.h"

typedef struct RwChunkHeader {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 libraryIDPack;
} RwChunkHeader;

typedef struct RwChunkHeaderInfo {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;
    RwUInt32 buildNum;
    RwBool isComplex;
} RwChunkHeaderInfo;

extern RwUInt32 RwStreamRead(RwStream*, void*, RwUInt32);
extern RwStream* RwStreamWrite(RwStream*, const void*, RwUInt32);
extern RwStream* RwStreamSkip(RwStream*, RwUInt32);
void* RwMemLittleEndian32(void*, RwUInt32);
void* RwMemNative32(void*, RwUInt32);

static RwBool ChunkIsComplex(const RwChunkHeaderInfo* chunkHeaderInfo) {
    RwBool result = 0;
    switch (chunkHeaderInfo->type) {
    case 1: result = 0; break;
    case 2: result = 0; break;
    case 3: result = 0; break;
    case 5: result = 1; break;
    case 6: result = 1; break;
    case 7: result = 1; break;
    case 8: result = 1; break;
    case 9: result = 1; break;
    case 10: result = 1; break;
    case 11: result = 1; break;
    case 13: result = 0; break;
    case 14: result = 1; break;
    case 15: result = 1; break;
    case 16: result = 1; break;
    case 18: result = 1; break;
    case 19: result = 0; break;
    case 20: result = 1; break;
    case 26: result = 1; break;
    default: result = 0; break;
    }
    return result;
}

RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* typeOut,
                                RwUInt32* lengthOut, RwUInt32* versionOut,
                                RwUInt32* buildNumOut) {
    RwChunkHeader header;
    RwChunkHeaderInfo info;
    RwBool success;

    success = RwStreamRead(stream, &header, sizeof(header)) == sizeof(header);
    if (!success) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001A);
        RwErrorSet(&error);
        return 0;
    }
    RwMemNative32(&header, sizeof(header));
    info.type = header.type;
    info.length = header.length;
    if ((header.libraryIDPack & 0xFFFF0000) == 0) {
        info.version = header.libraryIDPack << 8;
        info.buildNum = 0;
    } else {
        info.version = (((header.libraryIDPack >> 14) & 0x3FF00) + 0x30000) |
                       ((header.libraryIDPack >> 16) & 0x3F);
        info.buildNum = (RwUInt16)header.libraryIDPack;
    }
    info.isComplex = ChunkIsComplex(&info);
    if (typeOut != 0) {
        *typeOut = info.type;
    }
    if (lengthOut != 0) {
        *lengthOut = info.length;
    }
    if (buildNumOut != 0) {
        *buildNumOut = info.buildNum;
    }
    if (versionOut != 0) {
        *versionOut = info.version;
    }
    return 1;
}

RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type,
                                              RwInt32 size, RwUInt32 version,
                                              RwUInt32 buildNum) {
    RwChunkHeader header;
    RwStream* result;
    header.type = type;
    header.length = size;
    header.libraryIDPack =
        ((((version - 0x30000) & 0x3FF00) << 14) |
         ((version & 0x3F) << 16)) |
        (RwUInt16)buildNum;
    RwMemLittleEndian32(&header, sizeof(header));
    result = RwStreamWrite(stream, &header, sizeof(header));
    return result;
}




RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type,
                         RwUInt32* lengthOut, RwUInt32* versionOut) {
    RwUInt32 currentType;
    RwUInt32 length;
    RwUInt32 version;

    while (_rwStreamReadChunkHeader(stream, &currentType, &length, &version,
                                    0)) {
        if (currentType == type) {
            if (version < 0x34000) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x80000004);
                RwErrorSet(&error);
                return 0;
            }
            if (version > 0x36003) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x80000004);
                RwErrorSet(&error);
                return 0;
            }
            if (lengthOut != 0) {
                *lengthOut = length;
            }
            if (versionOut != 0) {
                *versionOut = version;
            }
            return 1;
        }
        if (RwStreamSkip(stream, length) == 0) {
            return 0;
        }
    }
    return 0;
}

void* RwMemLittleEndian32(void* memory, RwUInt32 size) {
    RwUInt32* words = memory;
    size >>= 2;
    while (size != 0) {
        *words = (*words << 24) |
                 (((*words * 0x100) & 0x00FF0000) |
                 ((*words >> 24) | ((*words / 0x100) & 0x0000FF00)));
        words++;
        size--;
    }
    return memory;
}

void* RwMemLittleEndian16(void* memory, RwUInt32 size) {
    RwUInt16* halves = memory;
    size >>= 1;
    while (size != 0) {
        *halves = (*halves >> 8) | (*halves << 8);
        halves++;
        size--;
    }
    return memory;
}

void* RwMemNative32(void* memory, RwUInt32 size) {
    RwUInt32* words = memory;
    size >>= 2;
    while (size != 0) {
        *words = (*words << 24) |
                 (((*words * 0x100) & 0x00FF0000) |
                  ((*words >> 24) | ((*words / 0x100) & 0x0000FF00)));
        words++;
        size--;
    }
    return memory;
}




RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals,
                            RwUInt32 numBytes) {
    RwUInt8 buffer[256];
    const RwUInt8* source = (const RwUInt8*)reals;
    while (numBytes != 0) {
        RwUInt32 chunkSize = numBytes >= sizeof(buffer) ? sizeof(buffer) : numBytes;
        memcpy(buffer, source, chunkSize);
        RwMemLittleEndian32(buffer, chunkSize);
        if (RwStreamWrite(stream, buffer, chunkSize) == 0) {
            return 0;
        }
        numBytes -= chunkSize;
        source += chunkSize;
    }
    return stream;
}




RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* integers,
                             RwUInt32 numBytes) {
    RwUInt8 buffer[256];
    const RwUInt8* source = (const RwUInt8*)integers;
    while (numBytes != 0) {
        RwUInt32 chunkSize = numBytes >= sizeof(buffer) ? sizeof(buffer) : numBytes;
        memcpy(buffer, source, chunkSize);
        RwMemLittleEndian32(buffer, chunkSize);
        if (RwStreamWrite(stream, buffer, chunkSize) == 0) {
            return 0;
        }
        numBytes -= chunkSize;
        source += chunkSize;
    }
    return stream;
}




RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals,
                           RwUInt32 numBytes) {
    if (RwStreamRead(stream, reals, numBytes) == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001A);
        RwErrorSet(&error);
        return 0;
    }
    RwMemNative32(reals, numBytes);
    return stream;
}




RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* integers,
                            RwUInt32 numBytes) {
    if (RwStreamRead(stream, integers, numBytes) == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001A);
        RwErrorSet(&error);
        return 0;
    }
    RwMemNative32(integers, numBytes);
    return stream;
}
