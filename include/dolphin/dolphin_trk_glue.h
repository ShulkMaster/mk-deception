#ifndef DOLPHIN_DOLPHIN_TRK_GLUE_H
#define DOLPHIN_DOLPHIN_TRK_GLUE_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int TRKPollUART(void);
int TRKReadUARTN(u8* destination, int size);
void EnableEXI2Interrupts(void);
int InitMetroTRKCommTable(int hardware_id);
void UnreserveEXI2Port(void);
void ReserveEXI2Port(void);

#ifdef __cplusplus
}
#endif

#endif
