#ifndef DOLPHIN_BASE_PPCARCH_H
#define DOLPHIN_BASE_PPCARCH_H

#define HID2 920
#define WPAR 921

unsigned long PPCMfhid2(void);
void PPCMthid2(unsigned long value);
unsigned long PPCMfwpar(void);
void PPCMtwpar(unsigned long value);
void PPCSync(void);

#endif
