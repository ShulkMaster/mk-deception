#ifndef RW_RWERROR_H
#define RW_RWERROR_H

#include "rw/rwplcore.h"

RwError* RwErrorSet(RwError* error);
RwError* RwErrorGet(RwError* error);
int _rwerror(int code, ...);

#endif
