#ifndef MKD_SOFDEC_UTY_MEM_H
#define MKD_SOFDEC_UTY_MEM_H

void UTY_MemcpyDword(unsigned int* destination,
                     const unsigned int* source, unsigned int count);
void UTY_MemsetDword(unsigned int* destination, unsigned int value,
                     unsigned int count);
void* MEM_Copy(void* destination, const void* source, unsigned long size);

#endif
