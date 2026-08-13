#ifndef RW_RWOBJECT_H
#define RW_RWOBJECT_H


typedef struct RwObject {
    unsigned char type;
    unsigned char subType;
    unsigned char flags;
    unsigned char privateFlags;
    void* parent;
} RwObject;

static inline void rwObjectInitialize(void* object, int objectType,
                                      int objectSubType)
{
    RwObject* header = (RwObject*)object;
    header->type = (unsigned char)objectType;
    header->subType = (unsigned char)objectSubType;
    header->flags = 0;
    header->privateFlags = 0;
    header->parent = 0;
}

typedef enum RwStreamType {
    rwSTREAMFILE = 1,
    rwSTREAMFILENAME = 2,
    rwSTREAMMEMORY = 3,
    rwSTREAMCUSTOM = 4
} RwStreamType;

typedef enum RwStreamAccessType {
    rwSTREAMREAD = 1,
    rwSTREAMWRITE = 2,
    rwSTREAMAPPEND = 3
} RwStreamAccessType;

typedef void (*RwStreamCloseCallBack)(void* data);
typedef unsigned int (*RwStreamReadCallBack)(void* data, void* buffer,
                                             unsigned int length);
typedef int (*RwStreamWriteCallBack)(void* data, const void* buffer,
                                     unsigned int length);
typedef int (*RwStreamSkipCallBack)(void* data, unsigned int offset);

typedef struct RwStreamCustom {
    RwStreamCloseCallBack close;
    RwStreamReadCallBack read;
    RwStreamWriteCallBack write;
    RwStreamSkipCallBack skip;
    void* data;
} RwStreamCustom;

typedef struct RwStreamMemory {
    unsigned int position;
    unsigned int length;
    unsigned char* start;
} RwStreamMemory;

typedef char RwStreamMemorySizeCheck[sizeof(RwStreamMemory) == 0x0C ? 1 : -1];


typedef struct RwStream {
    RwStreamType type;
    RwStreamAccessType accessType;
    unsigned int reserved;
    union {
        struct { void* file; } file;
        RwStreamMemory memory;
        RwStreamCustom custom;
    } data;
    int owned;
} RwStream;

#endif
