#ifndef MKD_SOFDEC_SFD_HEADER_H
#define MKD_SOFDEC_SFD_HEADER_H

typedef struct SfdHeaderState {
    int field_00;
    int field_04;
    int field_08;
    int field_0C;
    unsigned char unknown_0010[0x80];
    int field_90;
} SfdHeaderState;

typedef char SfdHeaderStateSizeCheck[
    sizeof(SfdHeaderState) == 0x94 ? 1 : -1];

void SFHDS_InitFhd(SfdHeaderState* state, int enabled);
void SFHDS_FinishFhd(SfdHeaderState* state);
void SFHDS_Init(void);
void SFHDS_Finish(void);

#endif
