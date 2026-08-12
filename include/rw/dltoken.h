#ifndef RW_DLTOKEN_H
#define RW_DLTOKEN_H

#include "rw/rwcore_types.h"

extern unsigned short _RwDlTokenCurrent;
extern unsigned short _RwDlTokenLastSeen;

int _rwDlTokenQueryDone(unsigned short token);

#endif
