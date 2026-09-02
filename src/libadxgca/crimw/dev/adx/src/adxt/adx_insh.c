#include "cri/adxt_internal.h"
#include "cri/sj.h"
#include "runtime/cstring.h"

typedef struct AdxSjeHandle AdxSjeHandle;

extern void ADXSJE_Init(void);
extern void ADXSJE_Finish(void);
extern AdxSjeHandle* ADXSJE_Create(int count, SJ** inputs, SJ* output);
extern void ADXSJE_Destroy(AdxSjeHandle* encoder);
extern void ADXSJE_SetConfigSfa(AdxSjeHandle* encoder, int channels,
                                int sample_rate, int sample_count);
extern void ADXSJE_Start(AdxSjeHandle* encoder);
extern void ADXSJE_Stop(AdxSjeHandle* encoder);
extern void ADXSJE_ExecServer(void);

unsigned char adxt_dmybuf[0x40];
unsigned char adxt_hdbuf[0x400];

void ADXT_InsertHdrSfa(ADXTHandle* decoder, int channels,
                       int sample_rate, int sample_count)
{
    SJCK destination;
    SJCK header;
    SJ* inputs[2];
    SJ* header_stream;
    SJ* destination_stream;
    AdxSjeHandle* encoder;

    ADXSJE_Init();
    header_stream = SJRBF_Create(adxt_hdbuf, sizeof(adxt_hdbuf), 0);
    inputs[0] = SJMEM_Create(adxt_dmybuf, 0x20);
    inputs[1] = SJMEM_Create(adxt_dmybuf + 0x20, 0x20);
    destination_stream = decoder->input_sj;
    encoder = ADXSJE_Create(2, inputs, header_stream);
    ADXSJE_SetConfigSfa(encoder, channels, sample_rate, sample_count);
    ADXSJE_Start(encoder);
    ADXSJE_ExecServer();

    header_stream->interface->get_chunk(header_stream, 1,
                                        sizeof(adxt_hdbuf), &header);
    if (header.len == 0) {
        for (;;) {
        }
    }
    destination_stream->interface->get_chunk(destination_stream, 0,
                                              header.len, &destination);
    if (destination.len < header.len) {
        for (;;) {
        }
    }
    memcpy(destination.data, header.data, header.len);
    header_stream->interface->put_chunk(header_stream, 0, &header);
    destination_stream->interface->put_chunk(destination_stream, 1,
                                              &destination);

    ADXSJE_Stop(encoder);
    ADXSJE_Destroy(encoder);
    header_stream->interface->destroy(header_stream);
    inputs[1]->interface->destroy(inputs[1]);
    inputs[0]->interface->destroy(inputs[0]);
    ADXSJE_Finish();
}
