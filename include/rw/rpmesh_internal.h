#ifndef RW_RPMESH_INTERNAL_H
#define RW_RPMESH_INTERNAL_H

#include "rw/rwplcore.h"

typedef struct RwFreeList RwFreeList;

typedef struct RpMeshGlobals {
    short nextSerialNum;
    unsigned short field_0x02;
    RwFreeList* triStripListEntryFreeList;
    unsigned char meshFlags[0x20];
    unsigned char primitiveType[6];
} RpMeshGlobals;

extern RwModuleInfo meshModule;

#endif
