#ifndef DOLPHIN_EXI_H
#define DOLPHIN_EXI_H

#include "dolphin/types.h"

typedef struct OSContext OSContext;

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

typedef void (*EXICallback)(s32 chan, OSContext* context);

typedef struct EXIControl {
    EXICallback exiCallback;
    EXICallback tcCallback;
    EXICallback extCallback;
    volatile u32 state;
    int immLen;
    unsigned char* immBuf;
    u32 dev;
    u32 id;
    s32 idTime;
    int items;
    struct {
        u32 dev;
        EXICallback callback;
    } queue[3];
} EXIControl;
typedef char EXIControlSizeCheck[sizeof(EXIControl) == 0x40 ? 1 : -1];

EXICallback EXISetExiCallback(s32 chan, EXICallback callback);
void EXIInit(void);
u32 EXIClearInterrupts(s32 chan, int exi, int tc, int ext);
int EXILock(s32 chan, u32 device, EXICallback callback);
int EXIUnlock(s32 chan);
int EXISelect(s32 chan, u32 device, u32 frequency);
int EXIDeselect(s32 chan);
int EXIImm(s32 chan, void* buffer, s32 length,
           u32 type, EXICallback callback);
int EXIImmEx(s32 chan, void* buffer, s32 length,
             u32 type);
int EXIDma(s32 chan, void* buffer, s32 length,
           u32 type, EXICallback callback);
int EXISync(s32 chan);
int EXIProbe(s32 chan);
s32 EXIProbeEx(s32 chan);
int EXIAttach(s32 chan, EXICallback callback);
int EXIDetach(s32 chan);
u32 EXIGetState(s32 chan);
s32 EXIGetID(s32 chan, u32 device,
             u32* id);
void __OSEnableBarnacle(s32 channel, u32 device);

#endif
