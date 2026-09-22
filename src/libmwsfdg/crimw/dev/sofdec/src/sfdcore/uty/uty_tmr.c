#include "dolphin/types.h"

#define UTY_BUS_CLOCK (*(u32*)0x800000F8)

extern u64 OSGetTime(void);

static s32 utytmr_ch = 0;
static volatile s32 utytmr_init_cnt = 0;
static u64 utytmr_unit;

u64 UTY_GetTmrUnit(void)
{
    return utytmr_unit;
}

/* TODO: [blocked] 44.130436%; retail uses an inline mftb time-base read, which needs explicit assembly authorization. */
s32 UTY_IsTmrVoid(void)
{
    if (utytmr_init_cnt > 0 && utytmr_ch != -1) {
        (void)OSGetTime();
    }
    return utytmr_unit == 1;
}

/* TODO: [blocked] 58.823530%; retail uses an inline mftb time-base read, which needs explicit assembly authorization. */
u64 UTY_GetTmr(void)
{
    if (utytmr_init_cnt <= 0 || utytmr_ch == -1) {
        return 0;
    }
    return OSGetTime();
}

/* TODO: [near miss] 91.666664%; volatile init-count state removes one reload,
 * but retail still uses a shorter decrement/branch CFG. */
void UTY_FinishTmr(void)
{
    utytmr_init_cnt--;
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
