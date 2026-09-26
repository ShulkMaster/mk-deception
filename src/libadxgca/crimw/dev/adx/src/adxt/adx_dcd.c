#include "cri/adx_dcd.h"
#include "dolphin/types.h"
#include "fdlibm.h"
#include "runtime/cstring.h"

extern float __float_nan;
extern double __frsqrte(double value);

typedef char AdxFloatWordSizeCheck[sizeof(float) == sizeof(unsigned int) ? 1 : -1];

static inline int classify_float(float value)
{
    unsigned int bits;

    /* Match CRI's PowerPC word view under MWCC; copy bytes on other compilers. */
#ifdef __MWERKS__
    bits = *(const unsigned int*)&value;
#else
    memcpy(&bits, &value, sizeof(bits));
#endif
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
        return __float_nan;
    }
    if (classify_float(value) == 1) {
        value = __float_nan;
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
                                     unsigned char* version,
                                     unsigned char* revision)
{
    if (buffer_len < 0x14) {
        return -1;
    }
    if (header->magic != 0x8000) {
        return -2;
    }
    if (header->copyright_offset < 0x10) {
        return -1;
    }
    *version = header->version;
    *revision = header->flags;
    return 0;
}

int ADX_CalcHdrInfoLen(int version, int extra_len, int block_size,
                       unsigned int alignment)
{
    if (version == 0) {
        return alignment *
                   ((unsigned int)(0x1B + extra_len + (int)strlen("(c)CRI") +
                                   block_size + alignment) /
                    alignment) -
               block_size;
    }

    return alignment *
               ((unsigned int)(0x33 + extra_len + (int)strlen("(c)CRI") +
                               block_size + alignment) /
                alignment) -
           block_size;
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

int ADX_DecodeInfoAinf(unsigned char* buffer, int buffer_len,
                       int* ainf_len, unsigned char ainf[16],
                       short* default_out_volume, short default_pan[2])
{
    unsigned char version;
    unsigned char revision;
    int result;
    int required_len;
    int ainf_offset;
    unsigned char* pointer;

    *ainf_len = 0;
    result = decode_info_header((AdxHeader*)buffer, buffer_len,
                                &version, &revision);
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
    pointer = (unsigned char*)((u32)ainf_offset + (u32)buffer);
    ainf_offset += 4;
    if (*(short*)(pointer + 2) != 0) {
        ainf_offset += 0x14;
    }

    if ((((u32)buffer[ainf_offset] << 24) |
         (buffer[ainf_offset + 1] << 16) |
         (buffer[ainf_offset + 2] << 8) |
         buffer[ainf_offset + 3]) != 0x41494E46) {
        return -2;
    }

    *ainf_len = *(int*)(buffer + ainf_offset + 4);
    memcpy(ainf, &buffer[ainf_offset + 8], 16);
    pointer = (unsigned char*)((u32)ainf_offset + (u32)buffer);
    *default_out_volume = *(short*)(pointer + 0x18);
    default_pan[0] = *(short*)(pointer + 0x1C);
    default_pan[1] = *(short*)(pointer + 0x1E);
    return 0;
}

int ADX_DecodeInfoExLoop(signed char* buffer, int buffer_len,
                         int* inserted_samples, short* num_loops,
                         short* loop_type, int* start_sample,
                         int* start_offset, int* end_sample, int* end_offset)
{
    unsigned char version;
    unsigned char revision;
    int result;
    int required_len;
    int loop_offset;

    *num_loops = 0;
    result = decode_info_header((AdxHeader*)buffer, buffer_len,
                                &version, &revision);
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
    unsigned char version;
    unsigned char revision;

    if (decode_info_header(header, buffer_len, &version, &revision) != 0) {
        return -1;
    }

    if (version >= 4) {
        if (buffer_len < 0x20) {
            return -1;
        }
        if (header->magic != 0x8000) {
            return -2;
        }
        if (header->copyright_offset < 0x1C) {
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
    if (header->magic != 0x8000) {
        return -2;
    }
    if (header->copyright_offset < 0x10) {
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
    if (header->magic != 0x8000) {
        return -2;
    }
    if (header->copyright_offset < 0x0E) {
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
    unsigned char* data = (unsigned char*)header;

    if (buffer_len < 0x10) {
        return -1;
    }
    if ((unsigned short)((data[0] << 8) | data[1]) != 0x8000) {
        return -2;
    }

    *data_len = (data[3] | (data[2] << 8)) + 4;
    *encoding = header->encoding;
    *block_size = header->block_size;
    *bits_per_sample = header->bits_per_sample;
    *channel_count = header->channel_count;
    *sample_rate = ((u32)header->sample_rate_0 << 24) |
                   (header->sample_rate_1 << 16) |
                   (header->sample_rate_2 << 8) | header->sample_rate_3;
    *total_samples = ((u32)header->total_samples_0 << 24) |
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
    float z;
    float a;
    float b;
    float d;
    float c;

    z = (float)cos(6.2831855f * (float)cutoff / (float)sample_rate);
    a = (float)adx_sqrt_positive(2.0) - z;
    b = (float)adx_sqrt_positive(2.0) - 1.0f;
    d = adx_sqrtf((a + b) * (a - b));
    c = (a - d) / b;
    *coefficient0 = (short)(4096.0f * (2.0f * c));
    *coefficient1 = (short)(4096.0f * (-c * c));
}
