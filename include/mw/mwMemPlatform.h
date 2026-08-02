#ifndef MW_MWMEMPLATFORM_H
#define MW_MWMEMPLATFORM_H

#include "platform/os_types.h"

extern OSHeapHandle GameCubeSystemHeap;

int privConsoleMemSystemInit(void);
unsigned long mwMemSystemGetAvailSize(void);
unsigned char* privGetOSMemory(unsigned long size);
void MEMPRINT(const char* format, ...);

#endif
