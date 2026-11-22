#ifndef DOLPHIN_AR_H
#define DOLPHIN_AR_H

#include "dolphin/types.h"

typedef void (*ARQCallback)(u32 request_address);
typedef void (*ARDMACallback)(void);
typedef struct ARQRequest {
    struct ARQRequest* next;
    u32 owner;
    u32 type;
    u32 priority;
    u32 source;
    u32 destination;
    u32 length;
    ARQCallback callback;
} ARQRequest;
typedef char ARQRequestSizeCheck[sizeof(ARQRequest) == 0x20 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

u32 ARInit(u32* stack_index, u32 entry_count);
u32 ARGetSize(void);
u32 ARGetBaseAddress(void);
u32 ARAlloc(u32 length);
u32 ARFree(u32* length);
int ARCheckInit(void);
ARDMACallback ARRegisterDMACallback(ARDMACallback callback);
u32 ARGetDMAStatus(void);
void ARStartDMA(u32 type, u32 main_memory,
                u32 aram, u32 length);
void ARReset(void);
void ARSetSize(void);
u32 ARGetInternalSize(void);
void ARClear(u32 flag);
void __ARClearInterrupt(void);
unsigned short __ARGetInterruptStatus(void);

#ifdef __cplusplus
}
#endif

#endif
