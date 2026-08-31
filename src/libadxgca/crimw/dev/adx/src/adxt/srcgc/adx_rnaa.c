#include "cri/sj.h"

typedef void (*ADXRNAErrorCallback)(void* object, const char* message);

extern void AXRNA_SetAdjsfreqFlg(void* rna);
extern void AXRNA_SetStmHdInfo(void* rna, void* stream_header);
extern int AXRNA_DiscardData(void* rna, int samples);
extern void AXRNA_SetBitPerSmpl(void* rna, int bits);
extern void AXRNA_SetOutPan(void* rna, int channel, int pan);
extern void AXRNA_SetOutVol(void* rna, int volume);
extern void AXRNA_SetSfreq(void* rna, int frequency);
extern void AXRNA_SetNumChan(void* rna, int channels);
extern void AXRNA_ExecServer(void);
extern int AXRNA_GetNumRoom(void* rna);
extern int AXRNA_GetNumData(void* rna);
extern void AXRNA_SetPlaySw(void* rna, int enabled);
extern void AXRNA_SetTransSw(void* rna, int enabled);
extern void AXRNA_Destroy(void* rna);
extern void* AXRNA_Create(SJ* stream, int max_channels);
extern void AXRNA_EntryErrFunc(ADXRNAErrorCallback callback, void* object);
extern void AXRNA_Finish(void);
extern void AXRNA_Init(void);

void ADXRNA_SetAdjsfreqFlg(void* rna) { AXRNA_SetAdjsfreqFlg(rna); }

int ADXRNA_SetStmHdInfo(void* rna, void* stream_header)
{
    AXRNA_SetStmHdInfo(rna, stream_header);
    return 0;
}

void ADXRNA_SetTotalNumSmpl(void* rna, int samples) {}

int ADXRNA_DiscardData(void* rna, int samples)
{
    return AXRNA_DiscardData(rna, samples);
}

void ADXRNA_SetBitPerSmpl(void* rna, int bits)
{
    AXRNA_SetBitPerSmpl(rna, bits);
}

void ADXRNA_SetOutPan(void* rna, int channel, int pan)
{
    AXRNA_SetOutPan(rna, channel, pan);
}

void ADXRNA_SetOutVol(void* rna, int volume) { AXRNA_SetOutVol(rna, volume); }

void ADXRNA_SetSfreq(void* rna, int frequency)
{
    AXRNA_SetSfreq(rna, frequency);
}

void ADXRNA_SetNumChan(void* rna, int channels)
{
    AXRNA_SetNumChan(rna, channels);
}

void ADXRNA_ExecServer(void) { AXRNA_ExecServer(); }

int ADXRNA_GetNumRoom(void* rna) { return AXRNA_GetNumRoom(rna); }

int ADXRNA_GetNumData(void* rna) { return AXRNA_GetNumData(rna); }

void ADXRNA_SetPlaySw(void* rna, int enabled)
{
    AXRNA_SetPlaySw(rna, enabled);
}

void ADXRNA_SetTransSw(void* rna, int enabled)
{
    AXRNA_SetTransSw(rna, enabled);
}

void ADXRNA_Destroy(void* rna)
{
    AXRNA_SetPlaySw(rna, 0);
    AXRNA_SetTransSw(rna, 0);
    AXRNA_Destroy(rna);
}

void* ADXRNA_Create(SJ* stream, int max_channels)
{
    return AXRNA_Create(stream, max_channels);
}

void ADXRNA_EntryErrFunc(ADXRNAErrorCallback callback, void* object)
{
    AXRNA_EntryErrFunc(callback, object);
}

void ADXRNA_Finish(void) { AXRNA_Finish(); }

void ADXRNA_Init(void) { AXRNA_Init(); }
