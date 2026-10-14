#ifndef MKD_RUNTIME___MEM_H
#define MKD_RUNTIME___MEM_H

#ifdef __cplusplus
extern "C" {
#endif

__declspec(section ".init") void* memcpy(void* destination, const void* source, unsigned long size);
__declspec(section ".init") void __fill_mem(void* destination, int value, unsigned long size);
__declspec(section ".init") void* memset(void* destination, int value, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
