#include "dolphin/base/PPCArch.h"
#include "runtime/asm_sequences.inc"

asm unsigned long PPCMfmsr(void) {
    SEQ_PPCMfmsr();
}

asm void PPCMtmsr(unsigned long value) {
    SEQ_PPCMtmsr();
}

asm unsigned long PPCMfhid0(void) {
    SEQ_PPCMfhid0();
}

asm void PPCMthid0(unsigned long value) {
    SEQ_PPCMthid0();
}

asm unsigned long PPCMfl2cr(void) {
    SEQ_PPCMfl2cr();
}

asm void PPCMtl2cr(unsigned long value) {
    SEQ_PPCMtl2cr();
}

asm void PPCSync(void) {
    SEQ_PPCSync();
}

__declspec(weak) asm void PPCHalt(void) {
    SEQ_PPCHalt();
}

asm void PPCMtmmcr0(unsigned long value) {
    SEQ_PPCMtmmcr0();
}

asm void PPCMtmmcr1(unsigned long value) {
    SEQ_PPCMtmmcr1();
}

asm void PPCMtpmc1(unsigned long value) {
    SEQ_PPCMtpmc1();
}

asm void PPCMtpmc2(unsigned long value) {
    SEQ_PPCMtpmc2();
}

asm void PPCMtpmc3(unsigned long value) {
    SEQ_PPCMtpmc3();
}

asm void PPCMtpmc4(unsigned long value) {
    SEQ_PPCMtpmc4();
}

asm unsigned long PPCMfhid2(void) {
    SEQ_PPCMfhid2();
}

asm void PPCMthid2(unsigned long value) {
    SEQ_PPCMthid2();
}

asm unsigned long PPCMfwpar(void) {
    SEQ_PPCMfwpar();
}

asm void PPCMtwpar(unsigned long value) {
    SEQ_PPCMtwpar();
}

void PPCDisableSpeculation(void)
{
    PPCMthid0(PPCMfhid0() | 0x200);
}

asm void PPCSetFpNonIEEEMode(void) {
    SEQ_PPCSetFpNonIEEEMode();
}
