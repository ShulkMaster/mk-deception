#include "dolphin/types.h"
#include "runtime/asm_sequences.inc"

#define UTY_BUS_CLOCK (*(u32*)0x800000F8)

extern u64 OSGetTime(void);

static s32 utytmr_ch = 0;
static volatile s32 utytmr_init_cnt = 0;
static u64 utytmr_unit;

u64 UTY_GetTmrUnit(void)
{
    return utytmr_unit;
}

s32 UTY_IsTmrVoid(void)
{
    if (utytmr_init_cnt > 0 && utytmr_ch != -1) {
        register u32 tmp, hi;

        asm { SEQ_UTY_IsTmrVoid_ReadTimeBase(hi, tmp) }
    }
    return utytmr_unit == 1;
}

u64 UTY_GetTmr(void)
{
    register u32 hi, lo, hi2;

    if (utytmr_init_cnt <= 0 || utytmr_ch == -1) {
        return 0;
    }
    asm { SEQ_UTY_GetTmr_ReadTimeBase(hi, lo, hi2) }
    return ((u64)hi << 32) | lo;
}

void UTY_FinishTmr(void)
{
    s32 cnt;

    utytmr_init_cnt--;
    cnt = utytmr_init_cnt;
    if (utytmr_init_cnt < 0) {
        utytmr_init_cnt = 0;
    }
}

void UTY_InitTmr(s32 channel)
{
    utytmr_init_cnt++;
    if (utytmr_init_cnt > 1) {
        if (utytmr_ch == channel) {
            return;
        }
    }
    utytmr_ch = channel;
    if (channel == -1) {
        utytmr_unit = 1;
        return;
    }
    utytmr_unit = UTY_BUS_CLOCK >> 2;
}
