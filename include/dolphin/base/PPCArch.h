#ifndef DOLPHIN_BASE_PPCARCH_H
#define DOLPHIN_BASE_PPCARCH_H

#include "dolphin/types.h"

#define HID2 920
#define WPAR 921

u32 PPCMfmsr(void);
void PPCMtmsr(u32 value);
u32 PPCMfhid0(void);
void PPCMthid0(u32 value);
u32 PPCMfl2cr(void);
void PPCMtl2cr(u32 value);
u32 PPCMfhid2(void);
void PPCMthid2(u32 value);
u32 PPCMfwpar(void);
void PPCMtwpar(u32 value);
u32 PPCMffpscr(void);
void PPCMtfpscr(u32 value);
void PPCMtdec(u32 value);
void PPCSync(void);
void PPCHalt(void);
void PPCMtmmcr0(u32 value);
void PPCMtmmcr1(u32 value);
void PPCMtpmc1(u32 value);
void PPCMtpmc2(u32 value);
void PPCMtpmc3(u32 value);
void PPCMtpmc4(u32 value);
void PPCDisableSpeculation(void);
void PPCSetFpNonIEEEMode(void);

#endif
