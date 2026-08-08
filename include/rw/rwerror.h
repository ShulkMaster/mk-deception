#ifndef RW_RWERROR_H
#define RW_RWERROR_H

#include "rw/rwplcore.h"

RwError* RwErrorSet(RwError* error);
RwError* RwErrorGet(RwError* error);
RwInt32 _rwerror(RwInt32 code, ...);

#endif
