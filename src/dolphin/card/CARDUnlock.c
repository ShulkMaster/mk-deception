#include "dolphin/cache.h"
#include "dolphin/card.h"
#include "dolphin/dsp.h"
#include "dolphin/exi.h"
#include "runtime/cstring.h"

#include "__card.h"

#define CARDFreq EXI_FREQ_16M

static u8 CardData[352] __attribute__((aligned(32))) = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x02, 0xFF, 0x00, 0x21, 0x13,
    0x06, 0x12, 0x03, 0x12, 0x04, 0x13, 0x05, 0x00, 0x92, 0x00, 0xFF,
    0x00, 0x88, 0xFF, 0xFF, 0x00, 0x89, 0xFF, 0xFF, 0x00, 0x8A, 0xFF,
    0xFF, 0x00, 0x8B, 0xFF, 0xFF, 0x8F, 0x00, 0x02, 0xBF, 0x00, 0x88,
    0x16, 0xFC, 0xDC, 0xD1, 0x16, 0xFD, 0x00, 0x00, 0x16, 0xFB, 0x00,
    0x01, 0x02, 0xBF, 0x00, 0x8E, 0x25, 0xFF, 0x03, 0x80, 0xFF, 0x00,
    0x02, 0x94, 0x00, 0x27, 0x02, 0xBF, 0x00, 0x8E, 0x1F, 0xDF, 0x24,
    0xFF, 0x02, 0x40, 0x0F, 0xFF, 0x00, 0x98, 0x04, 0x00, 0x00, 0x9A,
    0x00, 0x10, 0x00, 0x99, 0x00, 0x00, 0x8E, 0x00, 0x02, 0xBF, 0x00,
    0x94, 0x02, 0xBF, 0x86, 0x44, 0x02, 0xBF, 0x00, 0x88, 0x16, 0xFC,
    0xDC, 0xD1, 0x16, 0xFD, 0x00, 0x03, 0x16, 0xFB, 0x00, 0x01, 0x8F,
    0x00, 0x02, 0xBF, 0x00, 0x8E, 0x03, 0x80, 0xCD, 0xD1, 0x02, 0x94,
    0x00, 0x48, 0x27, 0xFF, 0x03, 0x80, 0x00, 0x01, 0x02, 0x95, 0x00,
    0x5A, 0x03, 0x80, 0x00, 0x02, 0x02, 0x95, 0x80, 0x00, 0x02, 0x9F,
    0x00, 0x48, 0x00, 0x21, 0x8E, 0x00, 0x02, 0xBF, 0x00, 0x8E, 0x25,
    0xFF, 0x02, 0xBF, 0x00, 0x8E, 0x25, 0xFF, 0x02, 0xBF, 0x00, 0x8E,
    0x25, 0xFF, 0x02, 0xBF, 0x00, 0x8E, 0x00, 0xC5, 0xFF, 0xFF, 0x03,
    0x40, 0x0F, 0xFF, 0x1C, 0x9F, 0x02, 0xBF, 0x00, 0x8E, 0x00, 0xC7,
    0xFF, 0xFF, 0x02, 0xBF, 0x00, 0x8E, 0x00, 0xC6, 0xFF, 0xFF, 0x02,
    0xBF, 0x00, 0x8E, 0x00, 0xC0, 0xFF, 0xFF, 0x02, 0xBF, 0x00, 0x8E,
    0x20, 0xFF, 0x03, 0x40, 0x0F, 0xFF, 0x1F, 0x5F, 0x02, 0xBF, 0x00,
    0x8E, 0x21, 0xFF, 0x02, 0xBF, 0x00, 0x8E, 0x23, 0xFF, 0x12, 0x05,
    0x12, 0x06, 0x02, 0x9F, 0x80, 0xB5, 0x00, 0x21, 0x27, 0xFC, 0x03,
    0xC0, 0x80, 0x00, 0x02, 0x9D, 0x00, 0x88, 0x02, 0xDF, 0x27, 0xFE,
    0x03, 0xC0, 0x80, 0x00, 0x02, 0x9C, 0x00, 0x8E, 0x02, 0xDF, 0x2E,
    0xCE, 0x2C, 0xCF, 0x00, 0xF8, 0xFF, 0xCD, 0x00, 0xF9, 0xFF, 0xC9,
    0x00, 0xFA, 0xFF, 0xCB, 0x26, 0xC9, 0x02, 0xC0, 0x00, 0x04, 0x02,
    0x9D, 0x00, 0x9C, 0x02, 0xDF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static u32 next = 1;

static inline int CARDRand(void)
{
    next = (next * 0x41C64E6D) + 0x3039;
    return (next / 0x10000) & 0x7FFF;
}

static inline void CARDSrand(unsigned int seed)
{
    next = seed;
}

static inline u32 exnor_1st(u32 data, u32 rshift)
{
    u32 wk;
    u32 work;
    u32 i;

    work = data;
    for (i = 0; i < rshift; ++i) {
        wk = ~(work ^ (work >> 7) ^ (work >> 15) ^ (work >> 23));
        work = (work >> 1) | ((wk << 30) & 0x40000000);
    }

    return work;
}

static inline u32 exnor(u32 data, u32 lshift)
{
    u32 wk;
    u32 work;
    u32 i;

    work = data;
    for (i = 0; i < lshift; ++i) {
        wk = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
        work = (work << 1) | ((wk >> 30) & 2);
    }

    return work;
}

static u32 bitrev(u32 data)
{
    u32 wk;
    u32 i;
    u32 k = 0;
    u32 j = 1;

    wk = 0;
    for (i = 0; i < 32; ++i) {
        if (i > 15) {
            if (i == 31) {
                wk |= ((data & (1u << 31)) >> 31) & 1;
            } else {
                wk |= (data & (1u << i)) >> j;
                j += 2;
            }
        } else {
            wk |= (data & (1u << i)) << (31 - i - k);
            ++k;
        }
    }

    return wk;
}

#define SEC_AD1(address) ((u8)(((address) >> 29) & 3))
#define SEC_AD2(address) ((u8)(((address) >> 21) & 0xFF))
#define SEC_AD3(address) ((u8)(((address) >> 19) & 3))
#define SEC_BA(address) ((u8)(((address) >> 12) & 0x7F))

static s32 ReadArrayUnlock(s32 chan, u32 data, void* buffer, s32 length,
                           int mode)
{
    CARDControl* card;
    BOOL error;
    u8 command[5];

    card = &__CARDBlock[chan];
    if (!EXISelect(chan, 0, CARDFreq)) {
        return CARD_RESULT_NOCARD;
    }

    data &= 0xFFFFF000;
    memset(command, 0, sizeof(command));
    command[0] = 0x52;
    if (mode == 0) {
        command[1] = SEC_AD1(data);
        command[2] = SEC_AD2(data);
        command[3] = SEC_AD3(data);
        command[4] = SEC_BA(data);
    } else {
        command[1] = (u8)(data >> 24);
        command[2] = (u8)(data >> 16);
    }

    error = FALSE;
    error |= !EXIImmEx(chan, command, sizeof(command), EXI_WRITE);
    error |= !EXIImmEx(chan, (u8*)card->workArea + sizeof(CARDID),
                       card->latency, EXI_WRITE);
    error |= !EXIImmEx(chan, buffer, length, EXI_READ);
    error |= !EXIDeselect(chan);

    return error ? CARD_RESULT_NOCARD : CARD_RESULT_READY;
}

static inline u32 GetInitVal(void)
{
    u32 value;
    u32 tick;

    tick = OSGetTick();
    CARDSrand(tick);
    value = 0x7FEC8000;
    value |= CARDRand();
    value &= 0xFFFFF000;
    return value;
}

static s32 DummyLen(void)
{
    u32 tick;
    u32 shift;
    s32 length;
    u32 attempts;

    shift = 1;
    attempts = 0;
    tick = OSGetTick();
    CARDSrand(tick);

    length = CARDRand();
    length &= 0x1F;
    ++length;
    while (length < 4 && attempts < 10) {
        tick = OSGetTick();
        length = (s32)(tick << shift);
        ++shift;
        if (shift > 16) {
            shift = 1;
        }
        CARDSrand((u32)length);
        length = CARDRand();
        length &= 0x1F;
        ++length;
        ++attempts;
    }

    if (length < 4) {
        length = 4;
    }

    return length;
}

static void InitCallback(void* taskInfo);
static void DoneCallback(void* taskInfo);

s32 __CARDUnlock(s32 chan, u8 flashID[12])
{
    u32 initValue;
    u32 data;
    s32 dummy;
    s32 length;
    u32 shift;
    u32 work;
    u32 feedback;
    u32 answer1 = 0;
    u32* word;
    u8 readBuffer[64];
    u32 parameter1A = 0;
    u32 parameter1B = 0;
    u32 parameter2A = 0;
    u32 parameter2B = 0;
    CARDControl* card;
    DSPTaskInfo* task;
    CARDDecParam* parameter;
    u8* input;
    u8* output;

    card = &__CARDBlock[chan];
    task = &card->task;
    parameter = (CARDDecParam*)card->workArea;
    input = (u8*)parameter + sizeof(CARDDecParam);
    input = (u8*)OSRoundUp32B(input);
    output = input + 32;

    initValue = GetInitVal();

    dummy = DummyLen();
    length = dummy;
    if (ReadArrayUnlock(chan, initValue, readBuffer, length, 0) < 0) {
        return CARD_RESULT_NOCARD;
    }

    shift = (u32)(dummy * 8 + 1);
    work = exnor_1st(initValue, shift);
    feedback = ~(work ^ (work >> 7) ^ (work >> 15) ^ (work >> 23));
    card->scramble = work | ((feedback << 31) & 0x80000000);
    card->scramble = bitrev(card->scramble);

    dummy = DummyLen();
    length = 20 + dummy;
    data = 0;
    if (ReadArrayUnlock(chan, data, readBuffer, length, 1) < 0) {
        return CARD_RESULT_NOCARD;
    }

    word = (u32*)readBuffer;
    parameter1A = *word++;
    parameter1B = *word++;
    answer1 = *word++;
    parameter2A = *word++;
    parameter2B = *word++;

    parameter1A ^= card->scramble;
    shift = 32;
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    parameter1B ^= card->scramble;
    shift = 32;
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    answer1 ^= card->scramble;
    shift = 32;
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    parameter2A ^= card->scramble;
    shift = 32;
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    parameter2B ^= card->scramble;
    shift = (u32)(dummy * 8);
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    shift = 33;
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    *(u32*)&input[0] = parameter2A;
    *(u32*)&input[4] = parameter2B;

    parameter->inputAddr = input;
    parameter->inputLength = 8;
    parameter->outputAddr = output;
    parameter->aramAddr = 0;

    DCFlushRange(input, 8);
    DCInvalidateRange(output, 4);
    DCFlushRange(parameter, sizeof(CARDDecParam));

    task->priority = 255;
    task->iramMemoryAddress = (u16*)OSCachedToPhysical(CardData);
    task->iramLength = sizeof(CardData);
    task->iramAddress = 0;
    task->initVector = 0x10;
    task->initCallback = InitCallback;
    task->resumeCallback = NULL;
    task->doneCallback = DoneCallback;
    task->requestCallback = NULL;
    DSPAddTask(task);

    word = (u32*)flashID;
    *word++ = parameter1A;
    *word++ = parameter1B;
    *word = answer1;

    return CARD_RESULT_READY;
}

static void InitCallback(void* taskInfo)
{
    s32 chan;
    CARDControl* card;
    DSPTaskInfo* task;
    CARDDecParam* parameter;

    task = taskInfo;
    for (chan = 0; chan < 2; ++chan) {
        card = &__CARDBlock[chan];
        if (&card->task == task) {
            break;
        }
    }

    parameter = (CARDDecParam*)card->workArea;

    DSPSendMailToDSP(0xFF000000);
    while (DSPCheckMailToDSP()) {
    }

    DSPSendMailToDSP((u32)parameter);
    while (DSPCheckMailToDSP()) {
    }
}

static void DoneCallback(void* taskInfo)
{
    u8 readBuffer[64];
    u32 data;
    s32 dummy;
    s32 length;
    u32 shift;
    u8 status;
    u32 work;
    u32 feedback;
    u32 answer2;
    s32 chan;
    CARDControl* card;
    s32 result;
    DSPTaskInfo* task;
    CARDDecParam* parameter;
    u8* input;
    u8* output;

    task = taskInfo;
    for (chan = 0; chan < 2; ++chan) {
        card = &__CARDBlock[chan];
        if (&card->task == task) {
            break;
        }
    }

    parameter = (CARDDecParam*)card->workArea;
    input = (u8*)parameter + sizeof(CARDDecParam);
    input = (u8*)OSRoundUp32B(input);
    output = input + 32;

    answer2 = *(u32*)output;
    dummy = DummyLen();
    length = dummy;
    data = (answer2 ^ card->scramble) & 0xFFFF0000;
    if (ReadArrayUnlock(chan, data, readBuffer, length, 1) < 0) {
        EXIUnlock(chan);
        __CARDMountCallback(chan, CARD_RESULT_NOCARD);
        return;
    }

    shift = (u32)((dummy + 4 + card->latency) * 8 + 1);
    work = exnor(card->scramble, shift);
    feedback = ~(work ^ (work << 7) ^ (work << 15) ^ (work << 23));
    card->scramble = work | ((feedback >> 31) & 1);

    dummy = DummyLen();
    length = dummy;
    data = ((answer2 << 16) ^ card->scramble) & 0xFFFF0000;
    if (ReadArrayUnlock(chan, data, readBuffer, length, 1) < 0) {
        EXIUnlock(chan);
        __CARDMountCallback(chan, CARD_RESULT_NOCARD);
        return;
    }

    result = __CARDReadStatus(chan, &status);
    if (!EXIProbe(chan)) {
        EXIUnlock(chan);
        __CARDMountCallback(chan, CARD_RESULT_NOCARD);
        return;
    }

    if (result == CARD_RESULT_READY && !(status & 0x40)) {
        EXIUnlock(chan);
        result = CARD_RESULT_IOERROR;
    }

    __CARDMountCallback(chan, result);
}
