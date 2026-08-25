#ifndef DOLPHIN_BASE_PPCARCH_H
#define DOLPHIN_BASE_PPCARCH_H

#define HID2 920
#define WPAR 921

unsigned long PPCMfmsr(void);
void PPCMtmsr(unsigned long value);
unsigned long PPCMfhid0(void);
unsigned long PPCMfl2cr(void);
void PPCMtl2cr(unsigned long value);
unsigned long PPCMfhid2(void);
void PPCMthid2(unsigned long value);
unsigned long PPCMfwpar(void);
void PPCMtwpar(unsigned long value);
unsigned long PPCMffpscr(void);
void PPCMtfpscr(unsigned long value);
void PPCMtdec(unsigned long value);
void PPCSync(void);
void PPCHalt(void);

#endif
