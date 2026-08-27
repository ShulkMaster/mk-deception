typedef struct AhxDecoder AhxDecoder;
typedef struct SjHandle SjHandle;

typedef struct AdxBasicAhx {
    /* The preceding ADXB fields are not accessed by this backend adapter. */
    unsigned char reserved_00[0xB4];
    AhxDecoder* decoder;
    int max_decoded_samples;
    int max_decoded_blocks;
} AdxBasicAhx;

typedef void (*AhxSetInputSjFunc)(AhxDecoder* decoder, SjHandle* input_sj);
typedef void (*AhxSetDecodedSamplesFunc)(AhxDecoder* decoder, int samples);
typedef void (*AhxTermSupplyFunc)(AhxDecoder* decoder);
typedef void (*AhxExecFunc)(AdxBasicAhx* adxb);

AhxSetInputSjFunc ahxsetsjifunc = 0;
AhxSetDecodedSamplesFunc ahxsetdecsmplfunc = 0;
AhxTermSupplyFunc ahxtermsupplyfunc = 0;
AhxExecFunc ahxexecfunc = 0;

void ADXB_AhxTermSupply(AdxBasicAhx* adxb)
{
    if (adxb->decoder != 0) {
        ahxtermsupplyfunc(adxb->decoder);
    }
}

void ADXB_ExecOneAhx(AdxBasicAhx* adxb)
{
    ahxexecfunc(adxb);
}

void ADXB_SetAhxDecSmpl(AdxBasicAhx* adxb, int samples)
{
    if (adxb->decoder != 0) {
        ahxsetdecsmplfunc(adxb->decoder, samples);
    }

    adxb->max_decoded_samples = samples;
    adxb->max_decoded_blocks = samples / 96;
}

void ADXB_SetAhxInSj(AdxBasicAhx* adxb, SjHandle* input_sj)
{
    if (adxb->decoder != 0) {
        ahxsetsjifunc(adxb->decoder, input_sj);
    }
}
