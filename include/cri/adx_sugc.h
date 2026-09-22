#ifndef CRI_ADX_SUGC_H
#define CRI_ADX_SUGC_H

typedef struct ADXGC_DVDFS_PRM {
    int read_mode;
} ADXGC_DVDFS_PRM;

typedef char ADXGC_DVDFS_PRMSizeCheck[
    sizeof(ADXGC_DVDFS_PRM) == 4 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

void ADXGC_SetupDvdFs(const ADXGC_DVDFS_PRM* parameters);

#ifdef __cplusplus
}
#endif

#endif
