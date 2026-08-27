typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed int s32;

#define UTY_BUS_CLOCK (*(u32*)0x800000F8)

extern u64 OSGetTime(void);

u64 utytmr_unit;
s32 utytmr_init_cnt;
s32 utytmr_ch;

u64 UTY_GetTmrUnit(void)
{
    return utytmr_unit;
}

s32 UTY_IsTmrVoid(void)
{
    if (utytmr_init_cnt > 0 && utytmr_ch != -1) {
        (void)OSGetTime();
    }
    return utytmr_unit == 1;
}

u64 UTY_GetTmr(void)
{
    if (utytmr_init_cnt <= 0 || utytmr_ch == -1) {
        return 0;
    }
    return OSGetTime();
}

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
    if (utytmr_init_cnt <= 1 || utytmr_ch != channel) {
        utytmr_ch = channel;
        if (channel == -1) {
            utytmr_unit = 1;
        } else {
            utytmr_unit = UTY_BUS_CLOCK >> 2;
        }
    }
}
