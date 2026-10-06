#include "runtime/asm_sequences.inc"

static const unsigned long __constants[] = {
    0x00000000, 0x00000000, 0x41F00000, 0x00000000, 0x41E00000, 0x00000000,
};

asm void *__cvt_fp2unsigned(void)
{
    SEQ___cvt_fp2unsigned();
}

void _savefpr_25(void);
void _savefpr_26(void);
void _savefpr_27(void);
void _savefpr_28(void);
void _savefpr_29(void);

asm void *__save_fpr(void)
{
    SEQ___save_fpr();
}

void _restfpr_25(void);
void _restfpr_26(void);
void _restfpr_27(void);
void _restfpr_28(void);
void _restfpr_29(void);

asm void *__restore_fpr(void)
{
    SEQ___restore_fpr();
}

void _savegpr_14(void);
void _savegpr_17(void);
void _savegpr_18(void);
void _savegpr_19(void);
void _savegpr_20(void);
void _savegpr_21(void);
void _savegpr_22(void);
void _savegpr_23(void);
void _savegpr_24(void);
void _savegpr_25(void);
void _savegpr_26(void);
void _savegpr_27(void);
void _savegpr_28(void);
void _savegpr_29(void);

asm void *__save_gpr(void)
{
    SEQ___save_gpr();
}

void _restgpr_14(void);
void _restgpr_17(void);
void _restgpr_18(void);
void _restgpr_19(void);
void _restgpr_20(void);
void _restgpr_21(void);
void _restgpr_22(void);
void _restgpr_23(void);
void _restgpr_24(void);
void _restgpr_25(void);
void _restgpr_26(void);
void _restgpr_27(void);
void _restgpr_28(void);
void _restgpr_29(void);

asm void *__restore_gpr(void)
{
    SEQ___restore_gpr();
}

asm void *__div2u(void)
{
    SEQ___div2u();
}

asm void *__div2i(void)
{
    SEQ___div2i();
}

asm void *__mod2u(void)
{
    SEQ___mod2u();
}

asm void *__shl2i(void)
{
    SEQ___shl2i();
}

asm void *__shr2u(void)
{
    SEQ___shr2u();
}

asm void *__shr2i(void)
{
    SEQ___shr2i();
}

asm void *__cvt_dbl_usll(void)
{
    SEQ___cvt_dbl_usll();
}
