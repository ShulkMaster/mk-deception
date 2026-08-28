#include "cri/adx_dcd.h"
#include "fdlibm.h"
#include "runtime/cstring.h"

typedef union AdxFloatRepresentation {
    float value;
    unsigned long bits;
} AdxFloatRepresentation;

extern unsigned long __float_nan[];
extern double __frsqrte(double value);

static inline int classify_float(float value)
{
    AdxFloatRepresentation representation;
    unsigned long bits;

    representation.value = value;
    bits = representation.bits;
    switch (bits & 0x7F800000) {
    case 0x7F800000:
        if ((bits & 0x007FFFFF) != 0) {
            return 1;
        }
        return 2;
    case 0:
        if ((bits & 0x007FFFFF) != 0) {
            return 5;
        }
        return 3;
    }
    return 4;
}

static inline float adx_sqrtf(float value)
{
    if (value > 0.0f) {
        double estimate = __frsqrte((double)value);

        estimate = 0.5 * estimate * (3.0 - estimate * estimate * value);
        estimate = 0.5 * estimate * (3.0 - estimate * estimate * value);
        estimate = 0.5 * estimate * (3.0 - estimate * estimate * value);
        return (float)(value * estimate);
    }
    if (value < 0.0) {
        return ((AdxFloatRepresentation*)__float_nan)[0].value;
    }
    if (classify_float(value) == 1) {
        value = ((AdxFloatRepresentation*)__float_nan)[0].value;
    }
    return value;
}

static inline double adx_sqrt_positive(double value)
{
    double estimate = __frsqrte(value);

    estimate = 0.5 * estimate * (3.0 - estimate * estimate * value);
    estimate = 0.5 * estimate * (3.0 - estimate * estimate * value);
    estimate = 0.5 * estimate * (3.0 - estimate * estimate * value);
    return value * estimate;
}

static inline int decode_info_header(AdxHeader* header, int buffer_len,
                                     unsigned int* version)
{
    int result = 0;

    if (buffer_len < 0x14) {
        result = -1;
    } else if (header->magic.value != 0x8000) {
        result = -2;
    } else if (header->copyright_offset.value < 0x10) {
        result = -1;
    } else {
        *version = header->version;
    }
    return result;
}

int ADX_CalcHdrInfoLen(int version, int extra_len, int block_size,
                       unsigned int alignment)
{
    if (version == 0) {
        int length = extra_len + (int)strlen("(c)CRI");
        length += block_size;
        length += alignment;
        length += 0x1B;
        return alignment * (length / alignment) - block_size;
    }

    {
        int length = extra_len + (int)strlen("(c)CRI");
        length += block_size;
        length += alignment;
        length += 0x33;
        return alignment * (length / alignment) - block_size;
    }
}

int ADX_DecodeFooter(signed char* buffer, int buffer_len,
                     short* data_len)
{
    if (buffer_len < 0x10) {
        return -1;
    }
    if (*(const unsigned short*)buffer != 0x8001) {
        return -2;
    }

    *data_len = *(const short*)(buffer + 2) + 4;
    return 0;
}

int ADX_DecodeInfoAinf(signed char* buffer, int buffer_len,
                       int* ainf_len, unsigned char ainf[16],
                       short* default_out_volume, short default_pan[2])
{
    unsigned int version;
    int result;
    int required_len;
    int ainf_offset;
    unsigned int tag;

    *ainf_len = 0;
    result = decode_info_header((AdxHeader*)buffer, buffer_len, &version);
    if (result != 0) {
        return result;
    }

    required_len = version == 4 ? 0x48 : 0x3C;
    if (buffer_len < required_len) {
        return -1;
    }
    if (*(const unsigned short*)buffer != 0x8000) {
        return -2;
    }
    if (*(const short*)(buffer + 2) < required_len - 4) {
        return -1;
    }

    ainf_offset = 0x14;
    if (version == 4) {
        ainf_offset = 0x20;
    }
    if (*(short*)(buffer + ainf_offset + 2) != 0) {
        ainf_offset += 0x14;
    }
    ainf_offset += 4;

    tag = ((unsigned int)(unsigned char)buffer[ainf_offset] << 24) |
          ((unsigned int)(unsigned char)buffer[ainf_offset + 1] << 16) |
          ((unsigned int)(unsigned char)buffer[ainf_offset + 2] << 8) |
          (unsigned char)buffer[ainf_offset + 3];
    ainf_offset += 4;
    if (tag != 0x41494E46) {
        return -2;
    }

    *ainf_len = *(int*)(buffer + ainf_offset);
    ainf_offset += 4;
    memcpy(ainf, buffer + ainf_offset, 16);
    ainf_offset += 16;
    *default_out_volume = *(short*)(buffer + ainf_offset);
    ainf_offset += 4;
    default_pan[0] = *(short*)(buffer + ainf_offset);
    ainf_offset += 2;
    default_pan[1] = *(short*)(buffer + ainf_offset);
    return 0;
}

int ADX_DecodeInfoExLoop(signed char* buffer, int buffer_len,
                         int* inserted_samples, short* num_loops,
                         short* loop_type, int* start_sample,
                         int* start_offset, int* end_sample, int* end_offset)
{
    unsigned int version;
    int result;
    int required_len;
    int loop_offset;

    *num_loops = 0;
    result = decode_info_header((AdxHeader*)buffer, buffer_len, &version);
    if (result != 0) {
        return result;
    }

    required_len = version == 4 ? 0x3C : 0x30;
    if (buffer_len < required_len) {
        return -1;
    }
    if (*(const unsigned short*)buffer != 0x8000) {
        return -2;
    }
    if (*(const short*)(buffer + 2) < required_len - 4) {
        return -1;
    }

    loop_offset = 0x14;
    if (version == 4) {
        loop_offset = 0x20;
    }
    *inserted_samples = *(const short*)(buffer + loop_offset);
    loop_offset += 2;
    *num_loops = *(const short*)(buffer + loop_offset);
    loop_offset += 2;
    if (*num_loops != 1) {
        return -2;
    }

    loop_offset += 2;
    *loop_type = *(const short*)(buffer + loop_offset);
    loop_offset += 2;
    *start_sample = *(const int*)(buffer + loop_offset);
    loop_offset += 4;
    *start_offset = *(const int*)(buffer + loop_offset);
    loop_offset += 4;
    *end_sample = *(const int*)(buffer + loop_offset);
    loop_offset += 4;
    *end_offset = *(const int*)(buffer + loop_offset);
    return 0;
}

int ADX_DecodeInfoExIdly(AdxHeader* header, int buffer_len,
                         short left_delay[2], short right_delay[2])
{
    unsigned int version;

    if (decode_info_header(header, buffer_len, &version) != 0) {
        return -1;
    }

    if (version >= 4) {
        if (buffer_len < 0x20) {
            return -1;
        }
        if (header->magic.value != 0x8000) {
            return -2;
        }
        if (header->copyright_offset.value < 0x1C) {
            return -1;
        }
        left_delay[0] = header->delay[0];
        right_delay[0] = header->delay[1];
        left_delay[1] = header->delay[2];
        right_delay[1] = header->delay[3];
    } else {
        left_delay[0] = right_delay[0] = left_delay[1] = right_delay[1] = 0;
    }
    return 0;
}

int ADX_DecodeInfoExVer(AdxHeader* header, int buffer_len,
                        unsigned char* version, unsigned char* revision)
{
    if (buffer_len < 0x14) {
        return -1;
    }
    if (header->magic.value != 0x8000) {
        return -2;
    }
    if (header->copyright_offset.value < 0x10) {
        return -1;
    }

    *version = header->version;
    *revision = header->flags;
    return 0;
}

int ADX_DecodeInfoExADPCM2(AdxHeader* header, int buffer_len,
                           short* coefficient)
{
    if (buffer_len < 0x12) {
        return -1;
    }
    if (header->magic.value != 0x8000) {
        return -2;
    }
    if (header->copyright_offset.value < 0x0E) {
        return -1;
    }

    *coefficient = header->highpass_frequency;
    return 0;
}

int ADX_DecodeInfo(AdxHeader* header, int buffer_len, short* data_len,
                   signed char* encoding, signed char* bits_per_sample,
                   signed char* block_size, signed char* channel_count,
                   int* sample_rate, int* total_samples,
                   int* samples_per_block)
{
    int header_code;

    if (buffer_len < 0x10) {
        return -1;
    }
    header_code = header->magic.bytes.low | (header->magic.bytes.high << 8);
    if (header_code != 0x8000) {
        return -2;
    }

    *data_len = (header->copyright_offset.bytes.low |
                 (header->copyright_offset.bytes.high << 8)) +
                4;
    *encoding = header->encoding;
    *block_size = header->block_size;
    *bits_per_sample = header->bits_per_sample;
    *channel_count = header->channel_count;
    *sample_rate = (header->sample_rate_0 << 24) |
                   (header->sample_rate_1 << 16) |
                   (header->sample_rate_2 << 8) | header->sample_rate_3;
    *total_samples = (header->total_samples_0 << 24) |
                     (header->total_samples_1 << 16) |
                     (header->total_samples_2 << 8) |
                     header->total_samples_3;
    if (*bits_per_sample == 0) {
        *samples_per_block = 0;
    } else {
        *samples_per_block = ((*block_size - 2) * 8) / *bits_per_sample;
    }
    return 0;
}

int ADX_ScanInfoCode(signed char* buffer, int buffer_len, short* data_len)
{
    short code;
    int offset;
    int minimum;

    minimum = 0x7FFFFFFF;
    code = (short)0x8000;

    for (offset = 0; offset < buffer_len - 1; offset += 2) {
        if (*(short*)&buffer[offset] == code) {
            minimum = offset < minimum ? offset : minimum;
            break;
        }
    }
    if (minimum != 0x7FFFFFFF) {
        *data_len = (short)minimum;
        return 0;
    } else {
        *data_len = 0;
        return -1;
    }
}

void ADX_GetCoefficient(int cutoff, int sample_rate, short* coefficient0,
                        short* coefficient1)
{
    float value1;
    float value2;
    float result;
    double cosine;

    cosine = cos((double)((6.2831855f * (float)cutoff) / (float)sample_rate));
    value1 = (float)adx_sqrt_positive(2.0) - (float)cosine;
    value2 = (float)adx_sqrt_positive(2.0) - 1.0f;
    result = (value1 -
              adx_sqrtf((value1 + value2) * (value1 - value2))) /
             value2;

    *coefficient0 = (short)(4096.0f * (2.0f * result));
    *coefficient1 = (short)((-result * result) * 4096.0f);
}
