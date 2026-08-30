#include "cri/adx_basic.h"

int memcmp(const void*, const void*, unsigned long);
#define NULL ((void*)0)

#define FORM 0x4D524F46
#define AIFF 0x46464941
#define COMM 0x4D4D4F43
#define SSND 0x444E5353

static inline unsigned int rd32(const unsigned char* p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static inline unsigned short rd16(const unsigned char* p)
{
    return p[0] | p[1] << 8;
}

static inline unsigned int sw32(unsigned int v)
{
    return v >> 24 | (v >> 8 & 0xFF00) | (v << 8 & 0xFF0000) | v << 24;
}

static inline unsigned short sw16(unsigned short v)
{
    return v >> 8 | v << 8;
}


static void* AIFF_GetInfo(void*, int*, int*, int*, int*);
void ADXB_ExecOneAiff8(AdxBasicDecoder*);
void ADXB_ExecOneAiff16(AdxBasicDecoder*);

void ADXB_ExecOneAiff(AdxBasicDecoder* d)
{
    if (d->codec_type == 1)
        ADXB_ExecOneAiff8(d);
    else
        ADXB_ExecOneAiff16(d);
}

void ADXB_ExecOneAiff8(AdxBasicDecoder* d)
{
    AdxDecodeParams* dp;
    unsigned char* input;
    unsigned short *pcm, *left, *right;
    int i, count;

    dp = &d->decode;
    input = (unsigned char*)dp->input;

    if (d->status == 1 && ADXPD_GetStat(d->expander) == 0) {
        d->get_write_info(d->get_write_object, &dp->write_position, &dp->room,
                          &dp->loop_samples);
        count = dp->pcm_size - dp->write_position;
        if (count > dp->room)
            count = dp->room;
        if (count > dp->input_blocks)
            count = dp->input_blocks;
        pcm = (unsigned short*)dp->pcm_buffer;
        left = &pcm[dp->write_position];
        if (d->channel_count == 2) {
            right = &pcm[dp->pcm_distance + dp->write_position];
            for (i = 0; i < count; i++) {
                left[i] = input[i * 2] * 256;
                right[i] = input[i * 2 + 1] * 256;
            }
        } else {
            for (i = 0; i < count; i++)
                left[i] = input[i] * 256;
        }
        d->decoded_samples = count;
        d->decoded_data_length = count * d->channel_count;
        d->status = 2;
    }
    if (d->status == 2) {
        d->add_write_info(d->add_write_object, d->decoded_data_length,
                          d->decoded_samples);
        d->status = 3;
    }
}

void ADXB_ExecOneAiff16(AdxBasicDecoder* d)
{
    AdxDecodeParams* dp;
    unsigned short *pcm, *left, *right, sample;
    unsigned short* input;
    int i, count;

    dp = &d->decode;
    input = (unsigned short*)dp->input;

    if (d->status == 1 && ADXPD_GetStat(d->expander) == 0) {
        d->get_write_info(d->get_write_object, &dp->write_position, &dp->room,
                          &dp->loop_samples);
        count = dp->pcm_size - dp->write_position;
        if (count > dp->room)
            count = dp->room;
        if (count > dp->input_blocks)
            count = dp->input_blocks;
        pcm = (unsigned short*)dp->pcm_buffer;
        left = &pcm[dp->write_position];
        if (d->channel_count == 2) {
            right = &pcm[dp->pcm_distance + dp->write_position];
            for (i = 0; i < count; i++) {
                sample = input[i * 2];
                left[i] = sample / 256 | sample * 256;
                sample = input[i * 2 + 1];
                right[i] = sample / 256 | sample * 256;
            }
        } else {
            for (i = 0; i < count; i++) {
                sample = input[i];
                left[i] = sample / 256 | sample * 256;
            }
        }
        d->decoded_samples = count;
        d->decoded_data_length = count * (d->channel_count << 1);
        d->status = 2;
    }
    if (d->status == 2) {
        d->add_write_info(d->add_write_object, d->decoded_data_length,
                          d->decoded_samples);
        d->status = 3;
    }
}

int ADXB_DecodeHeaderAiff(AdxBasicDecoder* d, signed char* input, int length)
{
    short data_length;
    int rate, channels, bits, samples, result;
    signed char* data;

    d->header_decoded = 1;
    if (length < 4096) {
        data_length = 0;
        result = -1;
    } else {
        data = AIFF_GetInfo(input, &rate, &channels, &bits, &samples);
        if (data == NULL) {
            result = -1;
        } else {
            data_length = data - input;
            if (data_length <= 0) {
                result = -1;
            } else {
                d->sample_rate = rate;
                d->channel_count = channels;
                d->bits_per_sample = bits;
                d->total_samples = samples;
                d->encoding = -1;
                d->block_length = d->channel_count * d->bits_per_sample / 8;
                d->samples_per_block = 1;
                result = 0;
            }
        }
    }
    if (result < 0)
        return 0;
    d->coefficient = 0;
    d->loop_count = d->loop_type = 0;
    d->loop_end_offset = d->loop_end_sample = d->loop_start_offset =
        d->loop_start_sample = d->loop_insert_samples = 0;
    d->decode.channel_count = d->channel_count;
    d->decode.block_size = d->block_length;
    d->decode.samples_per_block = d->samples_per_block;
    d->decode.pcm_buffer = d->pcm_buffer;
    d->decode.pcm_size = d->pcm_size;
    d->decode.pcm_distance = d->pcm_distance;
    d->decoded_data_length = d->decoded_samples = 0;
    d->format_type = 3;
    if (d->bits_per_sample == 8)
        d->codec_type = 1;
    else
        d->codec_type = 0;
    return data_length;
}

int ADXB_CheckAiff(const signed char* input)
{
    if (memcmp(input, "FORM", 4) == 0 && memcmp(input + 8, "AIFF", 4) == 0)
        return 1;
    return 0;
}

static void* AIFF_GetInfo(void* header, int* rate, int* channels, int* bits,
                          int* samples)
{
    unsigned char* p;
    unsigned char* end;
    int id;
    int size;
    int form;
    int have_comm;
    int have_ssnd;
    unsigned int value;
    void* data;

    have_ssnd = have_comm = 0;
    data = NULL;
    p = header;

    id = rd32(p); p += 4;
    size = rd32(p);
    size = sw32(size); p += 4;
    form = rd32(p); p += 4;
    if (id != FORM)
        return NULL;
    if (form != AIFF)
        return NULL;
    end = p + size - 4;
    while (p < end) {
        id = rd32(p); p += 4;
        size = rd32(p);
        size = sw32(size); p += 4;
        switch (id) {
        case COMM:
            if (!have_comm) {
                if (size < 18)
                    return NULL;
                value = rd16(p);
                *channels = sw16(value); p += 2;
                *samples = sw32(rd32(p)); p += 4;
                *bits = (short)sw16(rd16(p)); p += 2;
                value = sw16(rd16(p)); p += 2;
                *rate = sw16(rd16(p));
                *rate >>= 0x400E - value;
                p += 8;
                have_comm = 1;
                if (have_ssnd)
                    return data;
            }
            break;
        case SSND:
            if (!have_ssnd) {
                value = sw32(rd32(p)); p += 4;
                data = p + value;
                have_ssnd = 1;
                if (have_comm)
                    return data;
            }
            break;
        default:
            p += (size + 1) & ~1;
            break;
        }
    }
    return data;
}
