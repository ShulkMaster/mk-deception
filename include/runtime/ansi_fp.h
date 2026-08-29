#ifndef RUNTIME_ANSI_FP_H
#define RUNTIME_ANSI_FP_H

#include "dolphin/types.h"
#include "fdlibm.h"

#define SIGDIGLEN 36
#define CHAR_BIT 8
#define DBL_MANT_DIG 53

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

extern int __double_max[];
extern int __float_huge[];

#define DBL_MAX (*(f64*)__double_max)
#define INFINITY (*(f32*)__float_huge)
#define SIGNBIT(x) ((int)(__HI(x) & 0x80000000))

static inline int __fpclassifyf(f32 x) {
    switch ((*(s32*)&x) & 0x7F800000) {
    case 0x7F800000: {
        if ((*(s32*)&x) & 0x007FFFFF)
            return 1;
        return 2;
    }
    case 0: {
        if ((*(s32*)&x) & 0x007FFFFF)
            return 5;
        return 3;
    }
    }
    return 4;
}

static inline int __fpclassifyd(f64 x) {
    switch (__HI(x) & 0x7FF00000) {
    case 0x7FF00000: {
        if ((__HI(x) & 0x000FFFFF) || (__LO(x) & 0xFFFFFFFF))
            return 1;
        return 2;
    }
    case 0: {
        if ((__HI(x) & 0x000FFFFF) || (__LO(x) & 0xFFFFFFFF))
            return 5;
        return 3;
    }
    }
    return 4;
}

#define fpclassify(x) \
    ((sizeof(x) == sizeof(f32)) ? __fpclassifyf((f32)(x)) : __fpclassifyd((f64)(x)))
#define isinf(x) (fpclassify(x) == 2)
#define isfinite(x) (fpclassify(x) > 2)

typedef struct decimal {
    char sign;
    char reserved;
    s16 exp;
    struct {
        u8 length;
        u8 text[SIGDIGLEN];
        u8 reserved;
    } sig;
} decimal;

typedef struct decform {
    char style;
    char reserved;
    s16 digits;
} decform;

typedef char DecimalSizeCheck[sizeof(decimal) == 0x2A ? 1 : -1];
typedef char DecformSizeCheck[sizeof(decform) == 0x04 ? 1 : -1];

void __num2dec(const decform*, f64, decimal*);
f64 __dec2num(const decimal*);

#endif
