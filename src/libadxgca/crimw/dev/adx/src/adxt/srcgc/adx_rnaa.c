#include "cri/sj.h"

typedef void (*ADXRNAErrorCallback)(void* object, const char* message);
typedef struct AXRNAHandle AXRNAHandle;

extern void AXRNA_SetAdjsfreqFlg(AXRNAHandle* rna, int enabled);
extern void AXRNA_SetStmHdInfo(AXRNAHandle* rna, void* stream_header);
extern int AXRNA_DiscardData(AXRNAHandle* rna, int samples);
extern void AXRNA_SetBitPerSmpl(AXRNAHandle* rna, int bits);
extern void AXRNA_SetOutPan(AXRNAHandle* rna, int channel, int pan);
extern void AXRNA_SetOutVol(AXRNAHandle* rna, int volume);
extern void AXRNA_SetSfreq(AXRNAHandle* rna, int frequency);
extern void AXRNA_SetNumChan(AXRNAHandle* rna, int channels);
extern void AXRNA_ExecServer(void);
extern int AXRNA_GetNumRoom(AXRNAHandle* rna);
extern int AXRNA_GetNumData(AXRNAHandle* rna);
extern void AXRNA_SetPlaySw(AXRNAHandle* rna, int enabled);
extern void AXRNA_SetTransSw(AXRNAHandle* rna, int enabled);
extern void AXRNA_Destroy(AXRNAHandle* rna);
extern AXRNAHandle* AXRNA_Create(SJ** streams, int max_channels);
extern void AXRNA_EntryErrFunc(ADXRNAErrorCallback callback, void* object);
extern void AXRNA_Finish(void);
extern void AXRNA_Init(void);

void ADXRNA_SetAdjsfreqFlg(AXRNAHandle* rna, int enabled)
{
    AXRNA_SetAdjsfreqFlg(rna, enabled);
}

int ADXRNA_SetStmHdInfo(AXRNAHandle* rna, void* stream_header)
{
    AXRNA_SetStmHdInfo(rna, stream_header);
    return 0;
}

void ADXRNA_SetTotalNumSmpl(AXRNAHandle* rna, int samples) {}

int ADXRNA_DiscardData(AXRNAHandle* rna, int samples)
{
    return AXRNA_DiscardData(rna, samples);
}

void ADXRNA_SetBitPerSmpl(AXRNAHandle* rna, int bits)
{
    AXRNA_SetBitPerSmpl(rna, bits);
}

void ADXRNA_SetOutPan(AXRNAHandle* rna, int channel, int pan)
{
    AXRNA_SetOutPan(rna, channel, pan);
}

void ADXRNA_SetOutVol(AXRNAHandle* rna, int volume) { AXRNA_SetOutVol(rna, volume); }

void ADXRNA_SetSfreq(AXRNAHandle* rna, int frequency)
{
    AXRNA_SetSfreq(rna, frequency);
}

void ADXRNA_SetNumChan(AXRNAHandle* rna, int channels)
{
    AXRNA_SetNumChan(rna, channels);
}

void ADXRNA_ExecServer(void) { AXRNA_ExecServer(); }

int ADXRNA_GetNumRoom(AXRNAHandle* rna) { return AXRNA_GetNumRoom(rna); }

int ADXRNA_GetNumData(AXRNAHandle* rna) { return AXRNA_GetNumData(rna); }

void ADXRNA_SetPlaySw(AXRNAHandle* rna, int enabled)
{
    AXRNA_SetPlaySw(rna, enabled);
}

void ADXRNA_SetTransSw(AXRNAHandle* rna, int enabled)
{
    AXRNA_SetTransSw(rna, enabled);
}

void ADXRNA_Destroy(AXRNAHandle* rna)
{
    AXRNA_SetPlaySw(rna, 0);
    AXRNA_SetTransSw(rna, 0);
    AXRNA_Destroy(rna);
}

AXRNAHandle* ADXRNA_Create(SJ** streams, int max_channels, void* work)
{
    (void)work;
    return AXRNA_Create(streams, max_channels);
}

void ADXRNA_EntryErrFunc(ADXRNAErrorCallback callback, void* object)
{
    AXRNA_EntryErrFunc(callback, object);
}

void ADXRNA_Finish(void) { AXRNA_Finish(); }

void ADXRNA_Init(void) { AXRNA_Init(); }
