#ifndef DOLPHIN_AR_H
#define DOLPHIN_AR_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned long ARInit(unsigned long* stack_index, unsigned long entry_count);
unsigned long ARGetSize(void);
unsigned long ARGetBaseAddress(void);
unsigned long ARAlloc(unsigned long length);

#ifdef __cplusplus
}
#endif

#endif
