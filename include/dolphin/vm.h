#ifndef DOLPHIN_VM_H
#define DOLPHIN_VM_H

#ifdef __cplusplus
extern "C" {
#endif

void VMInit(unsigned long virtual_memory_size, unsigned long aram_base,
            unsigned long aram_size);
void VMQuit(void);
int VMAlloc(void* virtual_address, unsigned long size);

#ifdef __cplusplus
}
#endif

#endif
