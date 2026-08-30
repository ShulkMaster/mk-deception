#ifndef CRI_ADX_BASIC_H
#define CRI_ADX_BASIC_H
#include "cri/adx_xpnd.h"
typedef struct AdxDecodeParams {
    const unsigned short* input;
    int input_blocks;
    int channel_count;
    int block_size;
    int samples_per_block;
    short* pcm_buffer;
    int pcm_size;
    int pcm_distance;
    int write_position;
    int room;
    int loop_samples;
} AdxDecodeParams;
typedef void (*AdxGetWriteInfo)(void*, int*, int*, int*);
typedef void (*AdxAddWriteInfo)(void*, int, int);
typedef struct AdxBasicDecoder {
    short used;
    short header_decoded;
    int status;
    AdxXpnd* expander;
    signed char encoding;
    signed char bits_per_sample;
    signed char channel_count;
    signed char block_length;
    int samples_per_block;
    int sample_rate;
    int total_samples;
    short coefficient;
    short field_1E;
    int loop_insert_samples;
    short loop_count;
    short loop_type;
    int loop_start_sample;
    int loop_start_offset;
    int loop_end_sample;
    int loop_end_offset;
    int max_channels;
    short* pcm_buffer;
    int pcm_size;
    int pcm_distance;
    AdxDecodeParams decode;
    short field_74;
    short field_76;
    AdxGetWriteInfo get_write_info;
    void* get_write_object;
    AdxAddWriteInfo add_write_info;
    void* add_write_object;
    int total_decoded_samples;
    int current_write_position;
    int decoded_samples;
    int decoded_data_length;
    short format_type;
    short field_9A;
    short codec_type;
} AdxBasicDecoder;

typedef char AdxDecodeParamsSizeCheck[
    sizeof(AdxDecodeParams) == 0x2C ? 1 : -1];
typedef char AdxBasicDecoderSizeCheck[
    sizeof(AdxBasicDecoder) == 0xA0 ? 1 : -1];

int ADXB_CheckSpsd(const signed char*);
void ADXB_ExecOneSpsd(AdxBasicDecoder*);
int ADXB_DecodeHeaderSpsd(AdxBasicDecoder*, signed char*, int);
int ADX_DecodeInfoSpsd(signed char*, int, short*, signed char*, signed char*, signed char*, signed char*, int*, int*, int*, short*);
#endif
