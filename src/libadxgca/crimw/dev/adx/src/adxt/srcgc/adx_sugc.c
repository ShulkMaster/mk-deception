typedef void (*CvFsErrorCallback)(void* object, const char* message);

extern const unsigned char mfCiGetInterface;
extern const unsigned char gcCiGetInterface;
extern void cvFsEntryErrFunc(CvFsErrorCallback callback, void* object);
extern void cvFsAddDev(const char* name, const void* interface,
                       void* work);
extern void cvFsSetDefDev(const char* name);
extern void gcCiSetRdMode(int drive, int mode, int retries,
                          int read_mode);
extern void ADXERR_CallErrFunc1(const char* message);

const char* const adxgcsdk_build =
    "\nADXGCSDK Ver.20Apr2004Patch1 Build:Sep  3 2004 17:49:26\n";

void adxgc_err_dvd(void* object, const char* message);

void ADXGC_SetupDvdFs(const int* read_mode)
{
    cvFsEntryErrFunc(adxgc_err_dvd, 0);
    cvFsAddDev("MFS", &mfCiGetInterface, 0);
    cvFsEntryErrFunc(adxgc_err_dvd, 0);
    cvFsAddDev("GCD", &gcCiGetInterface, 0);
    cvFsSetDefDev("GCD");
    if (read_mode != 0) {
        gcCiSetRdMode(0, 0, 0, *read_mode);
    } else {
        gcCiSetRdMode(0, 0, 0, 0);
    }
}

void adxgc_err_dvd(void* object, const char* message)
{
    (void)object;
    ADXERR_CallErrFunc1(message);
}
