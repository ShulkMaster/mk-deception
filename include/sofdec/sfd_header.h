#ifndef MKD_SOFDEC_SFD_HEADER_H
#define MKD_SOFDEC_SFD_HEADER_H

#include "sofdec/sfd_player_types.h"

typedef struct SfdHeaderState {
    int field_00;
    int field_04;
    int field_08;
    int field_0C;
    unsigned char unknown_0010[0x6C];
    int field_7C;
    unsigned char unknown_0080[0x10];
    int field_90;
} SfdHeaderState;

typedef char SfdHeaderStateSizeCheck[
    sizeof(SfdHeaderState) == 0x94 ? 1 : -1];

void SFHDS_InitFhd(SfdHeaderState* state, int enabled);
void SFHDS_FinishFhd(SfdHeaderState* state);
int SFHDS_SetHdr(SfdHandle* handle, int stream_index,
                 const unsigned char* data, int size, int* header_flag);
int SFHDS_GetColType(SfdHandle* handle);
void SFHDS_Init(void);
void SFHDS_Finish(void);

#endif
