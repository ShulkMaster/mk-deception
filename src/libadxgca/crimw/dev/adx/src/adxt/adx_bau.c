#include "cri/adx_basic.h"

int memcmp(const void*, const void*, unsigned long);
#define NULL ((void*)0)

#define FORM 0x4D524F46
#define AU 0x46464941
#define COMM 0x4D4D4F43
#define SSND 0x444E5353
static short ulaw_exp_table[256] = { 0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84, 0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84, 0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84, 0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84, 0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804, 0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004, 0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444, 0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844, 0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64, 0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64, 0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74, 0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74, 0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC, 0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C, 0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0, 0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000, 0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C, 0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C, 0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C, 0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C, 0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC, 0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC, 0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC, 0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC, 0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C, 0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C, 0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C, 0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C, 0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104, 0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084, 0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040, 0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000 };

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


static void* AU_GetInfo(void*, int, int*, int*, int*, int*, int*);
void ADXB_ExecOneAuUlaw(AdxBasicDecoder*);
void ADXB_ExecOneAu8(AdxBasicDecoder*);
void ADXB_ExecOneAu16(AdxBasicDecoder*);

void ADXB_ExecOneAu(AdxBasicDecoder* d)
{
    if (d->codec_type == 2)
        ADXB_ExecOneAuUlaw(d);
    else if (d->codec_type == 1)
        ADXB_ExecOneAu8(d);
    else
        ADXB_ExecOneAu16(d);
}

void ADXB_ExecOneAuUlaw(AdxBasicDecoder* d)
{
    AdxDecodeParams* dp = &d->decode;
    unsigned char* input = (unsigned char*)dp->input;
    unsigned short *pcm, *left, *right;
    int i, count;

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
                left[i] = ulaw_exp_table[input[i * 2]];
                right[i] = ulaw_exp_table[input[i * 2 + 1]];
            }
        } else {
            for (i = 0; i < count; i++)
                left[i] = ulaw_exp_table[input[i]];
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

void ADXB_ExecOneAu8(AdxBasicDecoder* d)
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

void ADXB_ExecOneAu16(AdxBasicDecoder* d)
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

int ADXB_DecodeHeaderAu(AdxBasicDecoder* d, signed char* input, int length)
{
    short data_length;
    int rate, channels, bits, samples, codec, result;
    signed char* data;

    d->header_decoded = 1;
    if (length < 8) {
        data_length = 0;
        result = -1;
    } else {
        data = AU_GetInfo(input, length, &rate, &channels, &bits, &samples,
                          &codec);
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
    d->format_type = 4;
    d->codec_type = codec;
    return data_length;
}

int ADXB_CheckAu(const signed char* input)
{
    if (memcmp(input, ".snd", 4) == 0 || memcmp(input, ".sd", 4) == 0)
        return 1;
    return 0;
}

static void* AU_GetInfo(void* header, int length, int* rate, int* channels,
                        int* bits, int* samples, int* codec)
{
    unsigned char* p = header;
    unsigned int magic;
    unsigned int header_size;
    int data_size;
    unsigned int encoding;

    magic = rd32(p); p += 4;
    if (magic != 0x646E732E && magic != 0x64732E)
        return NULL;
    header_size = rd32(p);
    header_size = sw32(header_size); p += 4;
    if (length < (int)header_size)
        return NULL;
    data_size = rd32(p);
    data_size = sw32(data_size); p += 4;
    encoding = rd32(p);
    encoding = sw32(encoding); p += 4;
    switch (encoding) {
    case 1: *codec = 2; *bits = 8; break;
    case 2: *codec = 1; *bits = 8; break;
    case 3: *codec = 0; *bits = 16; break;
    default: return NULL;
    }
    *rate = rd32(p);
    *rate = sw32(*rate); p += 4;
    *channels = rd32(p);
    *channels = sw32(*channels); p += 4;
    if (*codec == 2)
        *samples = data_size / *channels;
    else if (*codec == 1)
        *samples = data_size / *channels;
    else if (*codec == 0)
        *samples = (data_size / 2) / *channels;
    else
        *samples = 0x7FFF0000;
    return (unsigned char*)header + header_size;
}
