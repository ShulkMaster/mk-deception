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

#define SWAP16(x) ((((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))

static unsigned char* AIFF_GetInfo(unsigned char*, int*, int*, int*, int*);
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
    unsigned short *pcm, *left, *right;
    unsigned short* input;
    unsigned short sample;
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
                left[i] = (sample << 8) | (sample >> 8);
                sample = input[i * 2 + 1];
                right[i] = (sample << 8) | (sample >> 8);
            }
        } else {
            for (i = 0; i < count; i++) {
                sample = input[i];
                left[i] = (sample << 8) | (sample >> 8);
            }
        }
        d->decoded_samples = count;
        d->decoded_data_length = d->channel_count * (count * 2);
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
    int samples, bits, channels, rate, result;
    signed char* data;

    d->header_decoded = 1;
    if (length < 4096) {
        data_length = 0;
        result = -1;
    } else {
        data = (signed char*)AIFF_GetInfo((unsigned char*)input, &rate,
                                         &channels, &bits, &samples);
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
    d->loop_type = 0;
    d->loop_count = 0;
    d->loop_end_offset = 0;
    d->loop_end_sample = 0;
    d->loop_start_offset = 0;
    d->loop_start_sample = 0;
    d->loop_insert_samples = 0;
    d->decode.channel_count = d->channel_count;
    d->decode.block_size = d->block_length;
    d->decode.samples_per_block = d->samples_per_block;
    d->decode.pcm_buffer = d->pcm_buffer;
    d->decode.pcm_size = d->pcm_size;
    d->decode.pcm_distance = d->pcm_distance;
    d->current_write_position = 0;
    d->total_decoded_samples = 0;
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

/* TODO: [near miss] 93.987180%; AIFF parse and byte-pack CFG agree;
 * remaining residue is register coloring. */
static unsigned char* AIFF_GetInfo(unsigned char* header, int* rate,
                                   int* channels, int* bits, int* samples)
{
    unsigned char* p;
    unsigned char* end;
    unsigned char* data;
    signed long id;
    signed long size;
    signed long form;
    signed long have_comm;
    signed long have_ssnd;
    unsigned short exp;
    unsigned short mant;
    unsigned long offset;

    p = header + 12;
    have_comm = 0;
    have_ssnd = 0;
    data = NULL;
    id = rd32(header);
    size = rd32(header + 4);
    size = sw32(size);
    form = rd32(header + 8);
    if (id != FORM)
        return NULL;
    if (form != AIFF)
        return NULL;
    end = p + size - 4;
    while (p < end) {
        id = rd32(p);
        size = rd32(p + 4);
        size = sw32(size);
        p += 8;
        switch (id) {
        case COMM:
            if (have_comm != 0)
                break;
            if (size < 18)
                return NULL;
            have_comm = 1;
            *channels = (p[0] & 0xFF) | ((p[1] & 0xFFFF) << 8);
            *channels = SWAP16(*channels);
            *samples = rd32(p + 2);
            *samples = sw32(*samples);
            *bits = (p[6] & 0xFF) | ((p[7] & 0xFFFF) << 8);
            *bits = SWAP16(*bits);
            exp = SWAP16((unsigned short)(p[8] | (p[9] << 8)));
            mant = SWAP16((unsigned short)(p[10] | (p[11] << 8)));
            p += 0x12;
            *rate = (signed long)mant >> (0x400E - exp);
            if (have_ssnd != 0)
                return data;
            break;
        case SSND:
            if (have_ssnd != 0)
                break;
            have_ssnd = 1;
            offset = rd32(p);
            offset = sw32(offset);
            p += 4;
            data = p + offset;
            if (have_comm != 0)
                return data;
            break;
        default:
            p += (size + 1) & ~1;
            break;
        }
    }
    return data;
}
