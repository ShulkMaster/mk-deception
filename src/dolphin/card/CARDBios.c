#include "dolphin/card.h"
#include "dolphin/dsp.h"
#include "dolphin/exi.h"

#include "__card.h"

#define CARDFreq EXI_FREQ_16M

#define OS_TIMER_CLOCK (__OSBusClock / 4)
#define OSMillisecondsToTicks(msec) ((msec) * (OS_TIMER_CLOCK / 1000))
#define OSSecondsToTicks(sec) ((sec) * OS_TIMER_CLOCK)

const char* __CARDVersion =
    "<< Dolphin SDK - CARD\trelease build: Apr  5 2004 04:15:35 (0x2301) >>";

CARDControl __CARDBlock[2];

static u16 __CARDEncode;
static u16 __CARDFastMode;

DVDDiskID __CARDDiskNone;

static void TimeoutHandler(OSAlarm* alarm, OSContext* context);
static inline void SetupTimeoutAlarm(CARDControl* card);
static s32 Retry(s32 chan);
static void UnlockedCallback(s32 chan, s32 result);
static s32 __CARDStart(s32 chan, CARDCallback txCallback,
                       CARDCallback exiCallback);
static BOOL OnReset(BOOL final);

static OSResetFunctionInfo ResetFunctionInfo = {OnReset, 127};

void __CARDDefaultApiCallback(s32 chan, s32 result)
{
}

void __CARDSyncCallback(s32 chan, s32 result)
{
    CARDControl* card;

    card = &__CARDBlock[chan];
    OSWakeupThread(&card->threadQueue);
}

void __CARDExtHandler(s32 chan, OSContext* context)
{
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];
    if (card->attached) {
        card->attached = FALSE;
        EXISetExiCallback(chan, NULL);
        OSCancelAlarm(&card->alarm);

        callback = card->exiCallback;
        if (callback != NULL) {
            card->exiCallback = NULL;
            callback(chan, CARD_RESULT_NOCARD);
        }

        if (card->result != CARD_RESULT_BUSY) {
            card->result = CARD_RESULT_NOCARD;
        }

        callback = card->extCallback;
        if (callback != NULL && card->mountStep >= CARD_MAX_MOUNT_STEP) {
            card->extCallback = NULL;
            callback(chan, CARD_RESULT_NOCARD);
        }
    }
}

/* Soft ceiling: retail's shared unlock block is 12 bytes smaller. */
void __CARDExiHandler(s32 chan, OSContext* context)
{
    CARDControl* card;
    CARDCallback callback;
    u8 status;
    s32 result;

    card = &__CARDBlock[chan];
    OSCancelAlarm(&card->alarm);
    if (!card->attached) {
        return;
    }

    if (!EXILock(chan, 0, NULL)) {
        result = CARD_RESULT_FATAL_ERROR;
    } else if ((result = __CARDReadStatus(chan, &status)) < 0 ||
               (result = __CARDClearStatus(chan)) < 0) {
        EXIUnlock(chan);
    } else {
        result = (status & 0x18) != 0 ? CARD_RESULT_IOERROR
                                     : CARD_RESULT_READY;
        if (result == CARD_RESULT_IOERROR && --card->retry > 0) {
            result = Retry(chan);
            if (result >= 0) {
                return;
            }
        } else {
            EXIUnlock(chan);
        }
    }

    callback = card->exiCallback;
    if (callback != NULL) {
        card->exiCallback = NULL;
        callback(chan, result);
    }
}

void __CARDTxHandler(s32 chan, OSContext* context)
{
    CARDControl* card;
    CARDCallback callback;
    int error;

    card = &__CARDBlock[chan];
    error = !EXIDeselect(chan);
    EXIUnlock(chan);

    callback = card->txCallback;
    if (callback != NULL) {
        card->txCallback = NULL;
        callback(chan, !error && EXIProbe(chan) ? CARD_RESULT_READY
                                                 : CARD_RESULT_NOCARD);
    }
}

void __CARDUnlockedHandler(s32 chan, OSContext* context)
{
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];
    callback = card->unlockCallback;
    if (callback != NULL) {
        card->unlockCallback = NULL;
        callback(chan, EXIProbe(chan) ? CARD_RESULT_UNLOCKED
                                      : CARD_RESULT_NOCARD);
    }
}

s32 __CARDEnableInterrupt(s32 chan, BOOL enable)
{
    BOOL error;
    u32 command;

    if (!EXISelect(chan, 0, CARDFreq)) {
        return CARD_RESULT_NOCARD;
    }

    command = enable ? 0x81010000 : 0x81000000;
    error = FALSE;
    error |= !EXIImm(chan, &command, 2, EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIDeselect(chan);
    return error ? CARD_RESULT_NOCARD : CARD_RESULT_READY;
}

s32 __CARDReadStatus(s32 chan, u8* status)
{
    BOOL error;
    u32 command;

    if (!EXISelect(chan, 0, CARDFreq)) {
        return CARD_RESULT_NOCARD;
    }

    command = 0x83000000;
    error = FALSE;
    error |= !EXIImm(chan, &command, 2, EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIImm(chan, status, 1, EXI_READ, NULL);
    error |= !EXISync(chan);
    error |= !EXIDeselect(chan);
    return error ? CARD_RESULT_NOCARD : CARD_RESULT_READY;
}

int __CARDReadVendorID(s32 chan, u16* vendorID)
{
    BOOL error;
    u32 command;

    if (!EXISelect(chan, 0, CARDFreq)) {
        return CARD_RESULT_NOCARD;
    }

    command = 0x85000000;
    error = FALSE;
    error |= !EXIImm(chan, &command, 2, EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIImm(chan, vendorID, 2, EXI_READ, NULL);
    error |= !EXISync(chan);
    error |= !EXIDeselect(chan);
    return error ? CARD_RESULT_NOCARD : CARD_RESULT_READY;
}

s32 __CARDClearStatus(s32 chan)
{
    BOOL error;
    u32 command;

    if (!EXISelect(chan, 0, CARDFreq)) {
        return CARD_RESULT_NOCARD;
    }

    command = 0x89000000;
    error = FALSE;
    error |= !EXIImm(chan, &command, 1, EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIDeselect(chan);
    return error ? CARD_RESULT_NOCARD : CARD_RESULT_READY;
}

static void TimeoutHandler(OSAlarm* alarm, OSContext* context)
{
    s32 chan;
    CARDControl* card;
    CARDCallback callback;

    for (chan = 0; chan < 2; ++chan) {
        card = &__CARDBlock[chan];
        if (alarm == &card->alarm) {
            break;
        }
    }

    if (!card->attached) {
        return;
    }

    EXISetExiCallback(chan, NULL);
    callback = card->exiCallback;
    if (callback != NULL) {
        card->exiCallback = NULL;
        callback(chan, CARD_RESULT_IOERROR);
    }
}

static inline void SetupTimeoutAlarm(CARDControl* card)
{
    OSCancelAlarm(&card->alarm);
    switch (card->cmd[0]) {
    case 0xF2:
        OSSetAlarm(&card->alarm, OSMillisecondsToTicks(100), TimeoutHandler);
        break;
    case 0xF3:
        break;
    case 0xF4:
        if (card->pageSize > 0x80) {
            OSSetAlarm(&card->alarm,
                       OSSecondsToTicks((OSTime)2) * (card->cBlock / 0x40),
                       TimeoutHandler);
            break;
        }
    case 0xF1:
        OSSetAlarm(&card->alarm,
                   OSSecondsToTicks((OSTime)2) *
                       (card->sectorSize / 0x2000),
                   TimeoutHandler);
        break;
    }
}

static s32 Retry(s32 chan)
{
    CARDControl* card;

    card = &__CARDBlock[chan];
    if (!EXISelect(chan, 0, CARDFreq)) {
        EXIUnlock(chan);
        return CARD_RESULT_NOCARD;
    }

    SetupTimeoutAlarm(card);

    if (!EXIImmEx(chan, card->cmd, card->cmdlen, EXI_WRITE)) {
        EXIDeselect(chan);
        EXIUnlock(chan);
        return CARD_RESULT_NOCARD;
    }

    if (card->cmd[0] == 0x52 &&
        !EXIImmEx(chan, (u8*)card->workArea + sizeof(CARDID), card->latency,
                  EXI_WRITE)) {
        EXIDeselect(chan);
        EXIUnlock(chan);
        return CARD_RESULT_NOCARD;
    }

    if (card->mode == 0xFFFFFFFF) {
        EXIDeselect(chan);
        EXIUnlock(chan);
        return CARD_RESULT_READY;
    }

    if (!EXIDma(chan, card->buffer,
                card->cmd[0] == 0x52 ? CARD_SEG_SIZE : card->pageSize,
                card->mode, __CARDTxHandler)) {
        EXIDeselect(chan);
        EXIUnlock(chan);
        return CARD_RESULT_NOCARD;
    }

    return CARD_RESULT_READY;
}

static void UnlockedCallback(s32 chan, s32 result)
{
    CARDCallback callback;
    CARDControl* card;

    card = &__CARDBlock[chan];
    if (result >= 0) {
        card->unlockCallback = UnlockedCallback;
        if (!EXILock(chan, 0, __CARDUnlockedHandler)) {
            result = CARD_RESULT_READY;
        } else {
            card->unlockCallback = NULL;
            result = Retry(chan);
        }
    }

    if (result < 0) {
        switch (card->cmd[0]) {
        case 0x52:
            callback = card->txCallback;
            if (callback != NULL) {
                card->txCallback = NULL;
                callback(chan, result);
            }
            break;
        case 0xF2:
        case 0xF4:
        case 0xF1:
            callback = card->exiCallback;
            if (callback != NULL) {
                card->exiCallback = NULL;
                callback(chan, result);
            }
            break;
        }
    }
}

static s32 __CARDStart(s32 chan, CARDCallback txCallback,
                       CARDCallback exiCallback)
{
    BOOL enabled;
    CARDControl* card;
    s32 result;

    enabled = OSDisableInterrupts();
    card = &__CARDBlock[chan];

    if (!card->attached) {
        result = CARD_RESULT_NOCARD;
    } else {
        if (txCallback != NULL) {
            card->txCallback = txCallback;
        }
        if (exiCallback != NULL) {
            card->exiCallback = exiCallback;
        }

        card->unlockCallback = UnlockedCallback;
        if (!EXILock(chan, 0, __CARDUnlockedHandler)) {
            result = CARD_RESULT_BUSY;
        } else {
            card->unlockCallback = NULL;
            if (!EXISelect(chan, 0, CARDFreq)) {
                EXIUnlock(chan);
                result = CARD_RESULT_NOCARD;
            } else {
                SetupTimeoutAlarm(card);
                result = CARD_RESULT_READY;
            }
        }
    }

    OSRestoreInterrupts(enabled);
    return result;
}

#define AD1(address) ((u8)(((address) >> 17) & 0x7F))
#define AD2(address) ((u8)(((address) >> 9) & 0xFF))
#define AD3(address) ((u8)(((address) >> 7) & 0x03))
#define BA(address) ((u8)((address)&0x7F))

s32 __CARDReadSegment(s32 chan, CARDCallback callback)
{
    CARDControl* card;
    s32 result;

    card = &__CARDBlock[chan];
    card->cmd[0] = 0x52;
    card->cmd[1] = AD1(card->addr);
    card->cmd[2] = AD2(card->addr);
    card->cmd[3] = AD3(card->addr);
    card->cmd[4] = BA(card->addr);
    card->cmdlen = 5;
    card->mode = EXI_READ;
    card->retry = 0;

    result = __CARDStart(chan, callback, NULL);
    if (result == CARD_RESULT_BUSY) {
        result = CARD_RESULT_READY;
    } else if (result >= 0) {
        if (!EXIImmEx(chan, card->cmd, card->cmdlen, EXI_WRITE) ||
            !EXIImmEx(chan, (u8*)card->workArea + sizeof(CARDID),
                      card->latency, EXI_WRITE) ||
            !EXIDma(chan, card->buffer, CARD_SEG_SIZE, card->mode,
                    __CARDTxHandler)) {
            card->txCallback = NULL;
            EXIDeselect(chan);
            EXIUnlock(chan);
            result = CARD_RESULT_NOCARD;
        } else {
            result = CARD_RESULT_READY;
        }
    }

    return result;
}

s32 __CARDWritePage(s32 chan, CARDCallback callback)
{
    CARDControl* card;
    s32 result;

    card = &__CARDBlock[chan];
    card->cmd[0] = 0xF2;
    if (card->pageSize > 0x80) {
        card->cmd[1] = AD1(card->addr) | 0x80;
    } else {
        card->cmd[1] = AD1(card->addr);
    }
    card->cmd[2] = AD2(card->addr);
    card->cmd[3] = AD3(card->addr);
    card->cmd[4] = BA(card->addr);
    card->cmdlen = 5;
    card->mode = EXI_WRITE;
    card->retry = 3;

    result = __CARDStart(chan, NULL, callback);
    if (result == CARD_RESULT_BUSY) {
        result = CARD_RESULT_READY;
    } else if (result >= 0) {
        if (!EXIImmEx(chan, card->cmd, card->cmdlen, EXI_WRITE) ||
            !EXIDma(chan, card->buffer, card->pageSize, card->mode,
                    __CARDTxHandler)) {
            card->exiCallback = NULL;
            EXIDeselect(chan);
            EXIUnlock(chan);
            result = CARD_RESULT_NOCARD;
        } else {
            result = CARD_RESULT_READY;
        }
    }

    return result;
}

s32 __CARDEraseSector(s32 chan, u32 address, CARDCallback callback)
{
    CARDControl* card;
    s32 result;

    card = &__CARDBlock[chan];
    if (card->pageSize > 0x80) {
        if (callback != NULL) {
            callback(chan, CARD_RESULT_READY);
        }
        return CARD_RESULT_READY;
    }

    card->cmd[0] = 0xF1;
    card->cmd[1] = AD1(address);
    card->cmd[2] = AD2(address);
    card->cmdlen = 3;
    card->mode = 0xFFFFFFFF;
    card->retry = 3;

    result = __CARDStart(chan, NULL, callback);
    if (result == CARD_RESULT_BUSY) {
        result = CARD_RESULT_READY;
    } else if (result >= 0) {
        if (!EXIImmEx(chan, card->cmd, card->cmdlen, EXI_WRITE)) {
            result = CARD_RESULT_NOCARD;
            card->exiCallback = NULL;
        } else {
            result = CARD_RESULT_READY;
        }

        EXIDeselect(chan);
        EXIUnlock(chan);
    }

    return result;
}

void CARDInit(void)
{
    int chan;

    if (__CARDBlock[0].diskID != NULL && __CARDBlock[1].diskID != NULL) {
        return;
    }

    __CARDEncode = OSGetFontEncode();
    OSRegisterVersion(__CARDVersion);
    DSPInit();
    OSInitAlarm();

    for (chan = 0; chan < 2; ++chan) {
        CARDControl* card = &__CARDBlock[chan];

        card->result = CARD_RESULT_NOCARD;
        OSInitThreadQueue(&card->threadQueue);
        OSCreateAlarm(&card->alarm);
    }

    __CARDSetDiskID((void*)OSPhysicalToCached(0));
    OSRegisterResetFunction(&ResetFunctionInfo);
}

u16 __CARDGetFontEncode(void)
{
    return __CARDEncode;
}

void __CARDSetDiskID(const DVDDiskID* diskID)
{
    __CARDBlock[0].diskID = diskID != NULL ? diskID : &__CARDDiskNone;
    __CARDBlock[1].diskID = diskID != NULL ? diskID : &__CARDDiskNone;
}

s32 __CARDGetControlBlock(s32 chan, CARDControl** cardOut)
{
    BOOL enabled;
    s32 result;
    CARDControl* card;

    card = &__CARDBlock[chan];
    if (chan < 0 || chan >= 2 || card->diskID == NULL) {
        return CARD_RESULT_FATAL_ERROR;
    }

    enabled = OSDisableInterrupts();
    if (!card->attached) {
        result = CARD_RESULT_NOCARD;
    } else if (card->result == CARD_RESULT_BUSY) {
        result = CARD_RESULT_BUSY;
    } else {
        card->result = CARD_RESULT_BUSY;
        result = CARD_RESULT_READY;
        card->apiCallback = NULL;
        *cardOut = card;
    }

    OSRestoreInterrupts(enabled);
    return result;
}

s32 __CARDPutControlBlock(CARDControl* card, s32 result)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    if (card->attached) {
        card->result = result;
    } else if (card->result == CARD_RESULT_BUSY) {
        card->result = result;
    }

    OSRestoreInterrupts(enabled);
    return result;
}

/* Retail keeps this SDK helper inline but omits its out-of-line copy. */
static inline s32 GetResultCode(s32 chan)
{
    CARDControl* card;

    if (chan < 0 || chan >= 2) {
        return CARD_RESULT_FATAL_ERROR;
    }
    card = &__CARDBlock[chan];
    return card->result;
}

s32 CARDFreeBlocks(s32 chan, s32* byteNotUsed, s32* filesNotUsed)
{
    CARDControl* card;
    s32 result;
    u16* fat;
    CARDDir* directory;
    CARDDir* entry;
    u16 fileNo;

    result = __CARDGetControlBlock(chan, &card);
    if (result < 0) {
        return result;
    }

    fat = __CARDGetFatBlock(card);
    directory = __CARDGetDirBlock(card);
    if (fat == NULL || directory == NULL) {
        return __CARDPutControlBlock(card, CARD_RESULT_BROKEN);
    }

    if (byteNotUsed != NULL) {
        *byteNotUsed = card->sectorSize * fat[CARD_FAT_FREEBLOCKS];
    }

    if (filesNotUsed != NULL) {
        *filesNotUsed = 0;
        for (fileNo = 0; fileNo < CARD_MAX_FILE; ++fileNo) {
            entry = &directory[fileNo];
            if (entry->fileName[0] == 0xFF) {
                ++*filesNotUsed;
            }
        }
    }

    return __CARDPutControlBlock(card, CARD_RESULT_READY);
}

s32 __CARDSync(s32 chan)
{
    CARDControl* card;
    s32 result;
    BOOL enabled;

    card = &__CARDBlock[chan];
    enabled = OSDisableInterrupts();
    while ((result = GetResultCode(chan)) == CARD_RESULT_BUSY) {
        OSSleepThread(&card->threadQueue);
    }

    OSRestoreInterrupts(enabled);
    return result;
}

static BOOL OnReset(BOOL final)
{
    if (!final) {
        if (CARDUnmount(0) == CARD_RESULT_BUSY ||
            CARDUnmount(1) == CARD_RESULT_BUSY) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CARDGetFastMode(void)
{
    return __CARDFastMode ? TRUE : FALSE;
}
