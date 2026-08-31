#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct ADX_AMP {
    s8 used;
    s8 stat;
    s8 maxnch;
    s8 reserved_03;
    SJ* sji[2];
    SJ* sjo[2];
    s32 total_exsmpl[2];
    s32 nch;
    s32 sfreq;
    f32 frm_len;
    f32 frm_prd;
    s32 frm_no;
} ADX_AMP;

typedef ADX_AMP* ADXAMP;

typedef char ADXAMPSizeCheck[sizeof(ADX_AMP) == 0x30 ? 1 : -1];

extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);

void ADXAMP_SetSfreq(ADXAMP amp, s32 sfreq)
{
    amp->sfreq = sfreq;
}

void ADXAMP_Stop(ADXAMP amp)
{
    amp->stat = 0;
}

void ADXAMP_Start(ADXAMP amp)
{
    SJCK chunk;
    SJ* stream;
    s32 channel;

    for (channel = 0; channel < amp->maxnch; channel++) {
        amp->total_exsmpl[channel] = 0;
    }

    amp->frm_no = 0;

    for (channel = 0; channel < amp->maxnch; channel++) {
        stream = amp->sji[channel];
        stream->interface->reset(stream);
        stream->interface->get_chunk(
            stream, 0, stream->interface->get_num_data(stream, 0), &chunk);
        memset(chunk.data, 0, chunk.len);
        stream->interface->unget_chunk(stream, 0, &chunk);
    }

    for (channel = 0; channel < amp->maxnch; channel++) {
        stream = amp->sjo[channel];
        stream->interface->reset(stream);
        stream->interface->get_chunk(
            stream, 0, stream->interface->get_num_data(stream, 0), &chunk);
        memset(chunk.data, 0, chunk.len);
        stream->interface->unget_chunk(stream, 0, &chunk);
    }

    amp->stat = 2;
}

void ADXAMP_Destroy(ADXAMP amp)
{
    if (amp != 0) {
        ADXCRS_Lock();
        memset(amp, 0, sizeof(*amp));
        ADXCRS_Unlock();
    }
}
