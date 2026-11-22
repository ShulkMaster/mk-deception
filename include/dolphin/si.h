#ifndef DOLPHIN_SI_H
#define DOLPHIN_SI_H

#include "dolphin/types.h"
#include "dolphin/os.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SI_MAX_CHAN 4
#define SI_MAX_TYPE 4
#define SI_COMCSR_IDX 13
#define SI_STATUS_IDX 14
#define SI_COMCSR_TCINT_MASK      (1UL << 31)
#define SI_COMCSR_TCINTMSK_MASK   (1UL << 30)
#define SI_COMCSR_COMERR_MASK     (1UL << 29)
#define SI_COMCSR_RDSTINT_MASK    (1UL << 28)
#define SI_COMCSR_RDSTINTMSK_MASK (1UL << 27)
#define SI_COMCSR_TSTART_MASK     1UL

#define SI_ERROR_UNDER_RUN 0x01
#define SI_ERROR_OVER_RUN 0x02
#define SI_ERROR_COLLISION 0x04
#define SI_ERROR_NO_RESPONSE 0x08
#define SI_ERROR_WRST 0x10
#define SI_ERROR_RDST 0x20
#define SI_ERROR_UNKNOWN 0x40
#define SI_ERROR_BUSY 0x80

#define SI_TYPE_MASK 0x18000000UL
#define SI_TYPE_N64 0x00000000UL
#define SI_TYPE_GC 0x08000000UL
#define SI_TYPE_DOLPHIN SI_TYPE_GC
#define SI_GC_WIRELESS 0x80000000UL
#define SI_GC_NOMOTOR 0x20000000UL
#define SI_GC_STANDARD 0x01000000UL
#define SI_WIRELESS_RECEIVED 0x40000000UL
#define SI_WIRELESS_IR 0x04000000UL
#define SI_WIRELESS_STATE 0x02000000UL
#define SI_WIRELESS_ORIGIN 0x00200000UL
#define SI_WIRELESS_FIX_ID 0x00100000UL
#define SI_WIRELESS_TYPE 0x000F0000UL
#define SI_WIRELESS_LITE_MASK 0x000C0000UL
#define SI_WIRELESS_LITE 0x00040000UL
#define SI_WIRELESS_CONT_MASK 0x00080000UL
#define SI_WIRELESS_CONT 0x00000000UL
#define SI_WIRELESS_ID 0x00C0FF00UL
#define SI_WIRELESS_TYPE_ID (SI_WIRELESS_TYPE | SI_WIRELESS_ID)
#define SI_N64_CONTROLLER 0x05000000UL
#define SI_N64_MIC 0x00010000UL
#define SI_N64_KEYBOARD 0x00020000UL
#define SI_N64_MOUSE 0x02000000UL
#define SI_GBA 0x00040000UL
#define SI_GC_CONTROLLER (SI_TYPE_GC | SI_GC_STANDARD)
#define SI_GC_RECEIVER (SI_TYPE_GC | SI_GC_WIRELESS)
#define SI_GC_WAVEBIRD (SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_STANDARD | SI_WIRELESS_STATE | SI_WIRELESS_FIX_ID)
#define SI_GC_KEYBOARD (SI_TYPE_GC | 0x00200000UL)
#define SI_GC_STEERING SI_TYPE_GC

typedef void (*SICallback)(s32 channel, u32 status, OSContext* context);
typedef void (*SITypeCallback)(s32 channel, u32 type);

typedef struct SIControl {
    s32 chan;
    u32 poll;
    u32 inputBytes;
    void* input;
    SICallback callback;
} SIControl;

typedef struct SIPacket {
    s32 chan;
    void* output;
    u32 outputBytes;
    void* input;
    u32 inputBytes;
    SICallback callback;
    OSTime fire;
} SIPacket;
typedef char SIControlSizeCheck[sizeof(SIControl) == 0x14 ? 1 : -1];
typedef char SIPacketSizeCheck[sizeof(SIPacket) == 0x20 ? 1 : -1];

int SIBusy(void);
int SIIsChanBusy(s32 channel);
int SIRegisterPollingHandler(__OSInterruptHandler handler);
int SIUnregisterPollingHandler(__OSInterruptHandler handler);
void SIInit(void);
u32 SIGetStatus(s32 channel);
void SISetCommand(s32 channel, u32 command);
void SITransferCommands(void);
u32 SISetXY(u32 x, u32 y);
u32 SIEnablePolling(u32 poll);
u32 SIDisablePolling(u32 poll);
int SIGetResponse(s32 channel, void* data);
int SITransfer(s32 channel, void* output, u32 outputBytes,
               void* input, u32 inputBytes, SICallback callback,
               OSTime delay);
u32 SIGetType(s32 channel);
u32 SIGetTypeAsync(s32 channel, SITypeCallback callback);
u32 SIDecodeType(u32 type);
u32 SIProbe(s32 channel);
extern u32 __PADFixBits;

void SISetSamplingRate(u32 rate);
void SIRefreshSamplingRate(void);

#ifdef __cplusplus
}
#endif

#endif
