#ifndef MW_MWMEMPLATFORM_H
#define MW_MWMEMPLATFORM_H

#include "dolphin/types.h"
#include "platform/os_types.h"

extern OSHeapHandle GameCubeSystemHeap;

int privConsoleMemSystemInit(void);
u32 mwMemSystemGetAvailSize(void);
unsigned char* privGetOSMemory(u32 size);
void MEMPRINT(const char* format, ...);

#endif
