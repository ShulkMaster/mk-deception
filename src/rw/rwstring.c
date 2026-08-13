#include "libmkparticle/rw_engine.h"
#include "rw/rwstring.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "rw/rwerror.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

extern char* strlwr(char*);
extern int sscanf(const char*, const char*, ...);

static const char nullString[] = "";

static int StrICmp(const char* string1, const char* string2) {
    char character1;
    char character2;

    if (string1 != 0 && string2 != 0) {
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
    }
    return 0;
}

static void StrUpr(char* string) {
    char* character;
    char value;
    if (string != 0) {
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

static void StrLwr(char* string) {
    char* character;
    char value;
    if (string != 0) {
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




static char* StrChr(const char* string, int findThis) {
    char* result = 0;
    char character;

    do {
        character = *string;
        if (character == (char)findThis) {
            result = (char*)string;
            break;
        }
        string++;
    } while (character != '\0');
    return result;
}




static char* StrRChr(const char* string, int findThis) {
    char* result = 0;
    char character;

    do {
        character = *string;
        if (character == (char)findThis) {
            result = (char*)string;
        }
        string++;
    } while (character != '\0');
    return result;
}

int _rwStringOpen(void) {
    RwEngineInstance->stringFuncs.sprintf = sprintf;
    RwEngineInstance->stringFuncs.vsprintf = vsprintf;
    RwEngineInstance->stringFuncs.strcpy = strcpy;
    RwEngineInstance->stringFuncs.strncpy = strncpy;
    RwEngineInstance->stringFuncs.strcat = strcat;
    RwEngineInstance->stringFuncs.strncat = strncat;
    RwEngineInstance->stringFuncs.strrchr = StrRChr;
    RwEngineInstance->stringFuncs.strchr = StrChr;
    RwEngineInstance->stringFuncs.strstr = strstr;
    RwEngineInstance->stringFuncs.strcmp = strcmp;
    RwEngineInstance->stringFuncs.strncmp = strncmp;
    RwEngineInstance->stringFuncs.stricmp = StrICmp;
    RwEngineInstance->stringFuncs.strlen = strlen;
    RwEngineInstance->stringFuncs.strupr = StrUpr;
    RwEngineInstance->stringFuncs.strlwr = StrLwr;
    RwEngineInstance->stringFuncs.strtok = strtok;
    RwEngineInstance->stringFuncs.sscanf = sscanf;
    return 1;
}

void _rwStringClose(void) {
}

int _rwStringStreamGetSize(const char* string) {
    if (string == 0) {
        string = nullString;
    }
    return (RwEngineInstance->stringFuncs.strlen(string) + 4) & ~3;
}




const char* _rwStringStreamWrite(const char* string, RwStream* stream) {
    unsigned int length;
    if (string == 0) {
        string = nullString;
    }
    length = _rwStringStreamGetSize(string);
    if (_rwStreamWriteVersionedChunkHeader(stream, 2, length, 0x36003,
                                           0xFFFF) == 0) {
        return 0;
    }
    if (RwStreamWrite(stream, string, length) == 0) {
        return 0;
    }
    return string;
}




static char* StringStreamRead(char* string, RwStream* stream,
                                unsigned int length) {
    unsigned char buffer[64] __attribute__((aligned(64)));
    char* destination;

    if (string == 0) {
        string = RwEngineInstance->fpMalloc(length, 0x30002);
        if (string == 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013, length);
            RwErrorSet(&error);
            return 0;
        }
    }
    destination = string;
    while (length != 0) {
        unsigned int chunkSize = length < sizeof(buffer) ? length : sizeof(buffer);
        unsigned int i;
        if (RwStreamRead(stream, buffer, chunkSize) != chunkSize) {
            return 0;
        }
        length -= chunkSize;
        for (i = 0; i < chunkSize; i++) {
            destination[i] = buffer[i];
        }
        destination += chunkSize;
    }
    return string;
}




static char* UnicodeStringStreamRead(char* string, RwStream* stream,
                                       unsigned int length) {
    unsigned short buffer[64] __attribute__((aligned(64)));
    char* destination;
    int allocated = 0;

    if (string == 0) {
        string = RwEngineInstance->fpMalloc(length, 0x30002);
        if (string == 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013, length);
            RwErrorSet(&error);
            return 0;
        }
        allocated = 1;
    }
    destination = string;
    while (length != 0) {
        unsigned int chunkSize;
        unsigned int characterCount;
        unsigned int i;
        if (length > sizeof(buffer)) {
            chunkSize = sizeof(buffer);
        } else {
            chunkSize = length;
        }
        if (RwStreamRead(stream, buffer, chunkSize) != chunkSize) {
            if (allocated) {
                RwEngineInstance->fpFree(string);
            }
            return 0;
        }
        length -= chunkSize;
        characterCount = chunkSize >> 1;
        for (i = 0; i < characterCount; i++) {
            destination[i] = (char)buffer[i];
        }
        destination += characterCount;
    }
    return string;
}




char* _rwStringStreamFindAndRead(char* string, RwStream* stream) {
    unsigned int type;
    unsigned int length;
    unsigned int version;

    while (_rwStreamReadChunkHeader(stream, &type, &length, &version, 0) !=
           0) {
        int validVersion = 0;
        if (version >= 0x34000 && version <= 0x36003) {
            validVersion = 1;
        }
        if (!validVersion) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000004);
            RwErrorSet(&error);
            return 0;
        }
        if (type == 2) {
            return StringStreamRead(string, stream, length);
        }
        if (type == 0x13) {
            return UnicodeStringStreamRead(string, stream, length);
        }
        if (RwStreamSkip(stream, length) == 0) {
            return 0;
        }
    }
    return 0;
}
