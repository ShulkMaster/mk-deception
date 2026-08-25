#ifndef DOLPHIN_AR_H
#define DOLPHIN_AR_H

typedef void (*ARQCallback)(unsigned long request_address);
typedef void (*ARDMACallback)(void);
typedef struct ARQRequest {
    struct ARQRequest* next;
    unsigned long owner;
    unsigned long type;
    unsigned long priority;
    unsigned long source;
    unsigned long destination;
    unsigned long length;
    ARQCallback callback;
} ARQRequest;
typedef char ARQRequestSizeCheck[sizeof(ARQRequest) == 0x20 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

unsigned long ARInit(unsigned long* stack_index, unsigned long entry_count);
unsigned long ARGetSize(void);
unsigned long ARGetBaseAddress(void);
unsigned long ARAlloc(unsigned long length);
unsigned long ARFree(unsigned long* length);
int ARCheckInit(void);
ARDMACallback ARRegisterDMACallback(ARDMACallback callback);
unsigned long ARGetDMAStatus(void);
void ARStartDMA(unsigned long type, unsigned long main_memory,
                unsigned long aram, unsigned long length);
void ARReset(void);
void ARSetSize(void);
unsigned long ARGetInternalSize(void);
void ARClear(unsigned long flag);
void __ARClearInterrupt(void);
unsigned short __ARGetInterruptStatus(void);
void __ARQPopTaskQueueHi(void);

#ifdef __cplusplus
}
#endif

#endif
