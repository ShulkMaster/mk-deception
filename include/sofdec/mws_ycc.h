#ifndef MKD_SOFDEC_MWS_YCC_H
#define MKD_SOFDEC_MWS_YCC_H

typedef struct MwsYccPlane {
    void* y;
    void* cb;
    void* cr;
    int y_pitch;
    int cb_pitch;
    int cr_pitch;
} MwsYccPlane;

typedef char MwsYccPlaneSizeCheck[sizeof(MwsYccPlane) == 0x18 ? 1 : -1];

void mwPlyCalcYccPlane(void* buffer, int width, int height, MwsYccPlane* output);

#endif
