#ifndef DOLPHIN_EXI_H
#define DOLPHIN_EXI_H

#include "dolphin/os.h"

#define EXI_READ 0
#define EXI_WRITE 1
#define EXI_FREQ_1M 0
#define EXI_FREQ_2M 1
#define EXI_FREQ_4M 2
#define EXI_FREQ_8M 3
#define EXI_FREQ_16M 4
#define EXI_FREQ_32M 5

#define EXI_MEMORY_CARD_59 0x00000004
#define EXI_MEMORY_CARD_123 0x00000008
#define EXI_MEMORY_CARD_251 0x00000010
#define EXI_MEMORY_CARD_507 0x00000020
#define EXI_MEMORY_CARD_1019 0x00000040
#define EXI_MEMORY_CARD_2043 0x00000080
#define EXI_USB_ADAPTER 0x01010000
#define EXI_NPDP_GDEV 0x01020000
#define EXI_MARLIN 0x03010000
#define EXI_MODEM 0x02020000
#define EXI_ETHER 0x04020200
#define EXI_RS232C 0x04040404
#define EXI_MIC 0x04060000
#define EXI_AD16 0x04120000
#define EXI_STREAM_HANGER 0x04130000
#define EXI_ETHER_VIEWER 0x04220001
#define EXI_IS_VIEWER 0x05070000

typedef void (*EXICallback)(signed long chan, OSContext* context);

typedef struct EXIControl {
    EXICallback exiCallback;
    EXICallback tcCallback;
    EXICallback extCallback;
    volatile unsigned long state;
    int immLen;
    unsigned char* immBuf;
    unsigned long dev;
    unsigned long id;
    signed long idTime;
    int items;
    struct {
        unsigned long dev;
        EXICallback callback;
    } queue[3];
} EXIControl;
typedef char EXIControlSizeCheck[sizeof(EXIControl) == 0x40 ? 1 : -1];

EXICallback EXISetExiCallback(signed long chan, EXICallback callback);
void EXIInit(void);
unsigned long EXIClearInterrupts(signed long chan, int exi, int tc, int ext);
int EXILock(signed long chan, unsigned long device, EXICallback callback);
int EXIUnlock(signed long chan);
int EXISelect(signed long chan, unsigned long device, unsigned long frequency);
int EXIDeselect(signed long chan);
int EXIImm(signed long chan, void* buffer, signed long length,
           unsigned long type, EXICallback callback);
int EXIImmEx(signed long chan, void* buffer, signed long length,
             unsigned long type);
int EXIDma(signed long chan, void* buffer, signed long length,
           unsigned long type, EXICallback callback);
int EXISync(signed long chan);
int EXIProbe(signed long chan);
signed long EXIProbeEx(signed long chan);
int EXIAttach(signed long chan, EXICallback callback);
int EXIDetach(signed long chan);
unsigned long EXIGetState(signed long chan);
signed long EXIGetID(signed long chan, unsigned long device,
                     unsigned long* id);
void __OSEnableBarnacle(signed long channel, unsigned long device);

#endif
