#ifndef MKD_RUNTIME___MEM_H
#define MKD_RUNTIME___MEM_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

__declspec(section ".init") void* memcpy(void* destination, const void* source, size_t size);
__declspec(section ".init") void __fill_mem(void* destination, int value, size_t size);
__declspec(section ".init") void* memset(void* destination, int value, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
