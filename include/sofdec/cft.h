#ifndef SOFDEC_CFT_H
#define SOFDEC_CFT_H

#include "dolphin/types.h"

/* Four consecutive 256-entry planes of packed YCC422 contributions. */
void CFT_MakeYcc422ColAdjTbl(void* table);

#endif
