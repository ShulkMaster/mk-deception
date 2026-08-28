#ifndef CRI_ADX_DCD_H
#define CRI_ADX_DCD_H

typedef union AdxHeaderMagic {
    unsigned short value;
    struct {
        unsigned char high;
        unsigned char low;
    } bytes;
} AdxHeaderMagic;

typedef union AdxHeaderOffset {
    short value;
    struct {
        unsigned char high;
        unsigned char low;
    } bytes;
} AdxHeaderOffset;

typedef struct AdxHeader {
    AdxHeaderMagic magic;
    AdxHeaderOffset copyright_offset;
    unsigned char encoding;
    unsigned char block_size;
    unsigned char bits_per_sample;
    unsigned char channel_count;
    unsigned char sample_rate_0;
    unsigned char sample_rate_1;
    unsigned char sample_rate_2;
    unsigned char sample_rate_3;
    unsigned char total_samples_0;
    unsigned char total_samples_1;
    unsigned char total_samples_2;
    unsigned char total_samples_3;
    unsigned short highpass_frequency;
    unsigned char version;
    unsigned char flags;
    unsigned char reserved[4];
    unsigned short delay[4];
} AdxHeader;

int ADX_CalcHdrInfoLen(int version, int extra_len, int block_size,
                       unsigned int alignment);
int ADX_DecodeFooter(signed char* buffer, int buffer_len,
                     short* data_len);
int ADX_DecodeInfoAinf(signed char* buffer, int buffer_len,
                       int* ainf_len, unsigned char ainf[16],
                       short* default_out_volume, short default_pan[2]);
int ADX_DecodeInfoExLoop(signed char* buffer, int buffer_len,
                         int* inserted_samples, short* num_loops,
                         short* loop_type, int* start_sample,
                         int* start_offset, int* end_sample, int* end_offset);
int ADX_DecodeInfoExIdly(AdxHeader* header, int buffer_len,
                         short left_delay[2], short right_delay[2]);
int ADX_DecodeInfoExVer(AdxHeader* header, int buffer_len,
                        unsigned char* version, unsigned char* revision);
int ADX_DecodeInfoExADPCM2(AdxHeader* header, int buffer_len,
                           short* coefficient);
int ADX_DecodeInfo(AdxHeader* header, int buffer_len, short* data_len,
                   signed char* encoding, signed char* bits_per_sample,
                   signed char* block_size, signed char* channel_count,
                   int* sample_rate, int* total_samples,
                   int* samples_per_block);
int ADX_ScanInfoCode(signed char* buffer, int buffer_len, short* data_len);
void ADX_GetCoefficient(int cutoff, int sample_rate, short* coefficient0,
                        short* coefficient1);

#endif
