#include "libmkparticle/rw_engine.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "rw/rwerror.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

extern RwChar* strncat(RwChar*, const RwChar*, size_t);
extern RwChar* strlwr(RwChar*);
extern int sscanf(const RwChar*, const RwChar*, ...);

static const RwChar nullString[] = "";

/* Near miss: retail-equivalent ASCII flow; only branch scheduling differs. */
static int StrICmp(const RwChar* string1, const RwChar* string2) {
    RwChar character1;
    RwChar character2;

    if (string1 == NULL || string2 == NULL) {
        return 0;
    }
    do {
        character1 = *string1;
        character2 = *string2;
        if (character1 >= 'A' && character1 <= 'Z') {
            character1 += 'a' - 'A';
        }
        if (character2 >= 'A' && character2 <= 'Z') {
            character2 += 'a' - 'A';
        }
        if (character1 != character2) {
            return character1 - character2;
        }
        string1++;
        string2++;
    } while (character1 != '\0' && character2 != '\0');
    if (character1 != character2) {
        return character1 - character2;
    }
    return 0;
}

static void StrUpr(RwChar* string) {
    RwChar* character;
    RwChar value;
    if (string != NULL) {
        character = string;
        while (*character != '\0') {
            value = *character;
            if (value >= 'a' && value <= 'z') {
                value -= 'a' - 'A';
                *character = value;
            }
            character++;
        }
    }
}

static void StrLwr(RwChar* string) {
    RwChar* character;
    RwChar value;
    if (string != NULL) {
        character = string;
        while (*character != '\0') {
            value = *character;
            if (value >= 'A' && value <= 'Z') {
                value += 'a' - 'A';
                *character = value;
            }
            character++;
        }
    }
}

/* Near miss: identical do-loop and byte comparison; parameter coloring differs. */
static RwChar* StrChr(const RwChar* string, int findThis) {
    RwChar* result = NULL;
    RwChar character;

    do {
        character = *string;
        if (character == (RwChar)findThis) {
            result = (RwChar*)string;
            break;
        }
        string++;
    } while (character != '\0');
    return result;
}

/* Near miss: identical do-loop and byte comparison; parameter coloring differs. */
static RwChar* StrRChr(const RwChar* string, int findThis) {
    RwChar* result = NULL;
    RwChar character;

    do {
        character = *string;
        if (character == (RwChar)findThis) {
            result = (RwChar*)string;
        }
        string++;
    } while (character != '\0');
    return result;
}

RwBool _rwStringOpen(void) {
    RwEngineInstance->stringFuncs.vecSprintf = sprintf;
    RwEngineInstance->stringFuncs.vecVsprintf = vsprintf;
    RwEngineInstance->stringFuncs.vecStrcpy = strcpy;
    RwEngineInstance->stringFuncs.vecStrncpy = strncpy;
    RwEngineInstance->stringFuncs.vecStrcat = strcat;
    RwEngineInstance->stringFuncs.vecStrncat = strncat;
    RwEngineInstance->stringFuncs.vecStrrchr = StrRChr;
    RwEngineInstance->stringFuncs.vecStrchr = StrChr;
    RwEngineInstance->stringFuncs.vecStrstr = strstr;
    RwEngineInstance->stringFuncs.vecStrcmp = strcmp;
    RwEngineInstance->stringFuncs.vecStrncmp = strncmp;
    RwEngineInstance->stringFuncs.vecStricmp = StrICmp;
    RwEngineInstance->stringFuncs.vecStrlen = strlen;
    RwEngineInstance->stringFuncs.vecStrupr = StrUpr;
    RwEngineInstance->stringFuncs.vecStrlwr = StrLwr;
    RwEngineInstance->stringFuncs.vecStrtok = strtok;
    RwEngineInstance->stringFuncs.vecSscanf = sscanf;
    return TRUE;
}

void _rwStringClose(void) {
}

RwInt32 _rwStringStreamGetSize(const RwChar* string) {
    if (string == NULL) {
        string = nullString;
    }
    return (RwEngineInstance->stringFuncs.vecStrlen(string) + 4) & ~3;
}

/* Near miss: body matches; only save/restore helper selection differs. */
const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream) {
    RwUInt32 length;
    if (string == NULL) {
        string = nullString;
    }
    length = _rwStringStreamGetSize(string);
    if (_rwStreamWriteVersionedChunkHeader(stream, 2, length, 0x36003,
                                           0xFFFF) == NULL) {
        return NULL;
    }
    if (RwStreamWrite(stream, string, length) == NULL) {
        return NULL;
    }
    return string;
}

/* Near miss: aligned-buffer algorithm matches; retail also stores an unused flag. */
static RwChar* StringStreamRead(RwChar* string, RwStream* stream,
                                RwUInt32 length) {
    RwUInt8 buffer[64] __attribute__((aligned(64)));
    RwChar* result = string;
    RwChar* destination;

    if (result == NULL) {
        result = RwEngineInstance->fpMalloc(length, 0x30002);
        if (result == NULL) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013, length);
            RwErrorSet(&error);
            return NULL;
        }
    }
    destination = result;
    while (length != 0) {
        RwUInt32 chunkSize = length < sizeof(buffer) ? length : sizeof(buffer);
        RwUInt32 i;
        if (RwStreamRead(stream, buffer, chunkSize) != chunkSize) {
            return NULL;
        }
        length -= chunkSize;
        for (i = 0; i < chunkSize; i++) {
            destination[i] = buffer[i];
        }
        destination += chunkSize;
    }
    return result;
}

/* Near miss: identical aligned conversion loop; local register allocation differs. */
static RwChar* UnicodeStringStreamRead(RwChar* string, RwStream* stream,
                                       RwUInt32 length) {
    RwUInt16 buffer[64] __attribute__((aligned(64)));
    RwChar* result = string;
    RwChar* destination;
    RwBool allocated = FALSE;

    if (result == NULL) {
        result = RwEngineInstance->fpMalloc(length, 0x30002);
        if (result == NULL) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013, length);
            RwErrorSet(&error);
            return NULL;
        }
        allocated = TRUE;
    }
    destination = result;
    while (length != 0) {
        RwUInt32 chunkSize = length < sizeof(buffer) ? length : sizeof(buffer);
        RwUInt32 characterCount;
        RwUInt32 i;
        if (RwStreamRead(stream, buffer, chunkSize) != chunkSize) {
            if (allocated) {
                RwEngineInstance->fpFree(result);
            }
            return NULL;
        }
        length -= chunkSize;
        characterCount = chunkSize >> 1;
        for (i = 0; i < characterCount; i++) {
            destination[i] = (RwChar)buffer[i];
        }
        destination += characterCount;
    }
    return result;
}

/* Near miss: exact dispatch/error CFG; stack slots and register coloring differ. */
RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream) {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;
    RwBool validVersion;

    for (;;) {
        if (_rwStreamReadChunkHeader(stream, &type, &length, &version, NULL) ==
            NULL) {
            return NULL;
        }
        validVersion = FALSE;
        if (version >= 0x34000 && version <= 0x36003) {
            validVersion = TRUE;
        }
        if (!validVersion) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000004);
            RwErrorSet(&error);
            return NULL;
        }
        if (type == 2) {
            return StringStreamRead(string, stream, length);
        }
        if (type == 0x13) {
            return UnicodeStringStreamRead(string, stream, length);
        }
        if (RwStreamSkip(stream, length) == NULL) {
            return NULL;
        }
    }
}
