#ifndef DOLPHIN_EXI_H
#define DOLPHIN_EXI_H

#include "dolphin/os.h"

#define EXI_READ 0
#define EXI_WRITE 1
#define EXI_FREQ_16M 4

typedef void (*EXICallback)(signed long chan, OSContext* context);

EXICallback EXISetExiCallback(signed long chan, EXICallback callback);
int EXILock(signed long chan, unsigned long device, EXICallback callback);
int EXIUnlock(signed long chan);
int EXISelect(signed long chan, unsigned long device, unsigned long frequency);
int EXIDeselect(signed long chan);
int EXIImm(signed long chan, void* buffer, signed long length,
           unsigned long type, EXICallback callback);
int EXIImmEx(signed long chan, void* buffer, signed long length,
             unsigned long type);
int EXIDma(signed long chan, void* buffer, signed long length,
           unsigned long type, EXICallback callback);
int EXISync(signed long chan);
int EXIProbe(signed long chan);
signed long EXIProbeEx(signed long chan);
int EXIAttach(signed long chan, EXICallback callback);
int EXIDetach(signed long chan);
unsigned long EXIGetState(signed long chan);
signed long EXIGetID(signed long chan, unsigned long device,
                     unsigned long* id);

#endif
