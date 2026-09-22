#include "cri/adx_sugc.h"

typedef struct CvFsInterface CvFsInterface;
typedef void (*CvFsErrorCallback)(void* object, const char* message,
                                  void* handle);
typedef CvFsInterface* (*CvFsInterfaceFactory)(void);

extern CvFsInterface* mfCiGetInterface(void);
extern CvFsInterface* gcCiGetInterface(void);
extern void cvFsEntryErrFunc(CvFsErrorCallback callback, void* object);
extern void cvFsAddDev(char* name, CvFsInterfaceFactory factory, void* work);
extern void cvFsSetDefDev(char* name);
extern void gcCiSetRdMode(int drive, int mode, int retries,
                          int read_mode);
extern void ADXERR_CallErrFunc1(const char* message);

const char* const volatile adxgcsdk_build =
    "\nADXGCSDK Ver.20Apr2004Patch1 Build:Sep  3 2004 17:49:26\n";

void adxgc_err_dvd(void* object, const char* message, void* handle);

void ADXGC_SetupDvdFs(const ADXGC_DVDFS_PRM* read_mode)
{
    adxgcsdk_build;
    cvFsEntryErrFunc(adxgc_err_dvd, 0);
    cvFsAddDev("MFS", mfCiGetInterface, 0);
    cvFsEntryErrFunc(adxgc_err_dvd, 0);
    cvFsAddDev("GCD", gcCiGetInterface, 0);
    cvFsSetDefDev("GCD");
    if (read_mode != 0) {
        gcCiSetRdMode(0, 0, 0, read_mode->read_mode);
    } else {
        gcCiSetRdMode(0, 0, 0, 0);
    }
}

void adxgc_err_dvd(void* object, const char* message, void* handle)
{
    (void)object;
    (void)handle;
    ADXERR_CallErrFunc1(message);
}
