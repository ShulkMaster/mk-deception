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
    int field_74;
    AdxGetWriteInfo get_write_info;
    void* get_write_object;
    AdxAddWriteInfo add_write_info;
    void* add_write_object;
    int total_decoded_samples;
    int current_write_position;
    int decoded_samples;
    int decoded_data_length;
    short format_type;
    short raw_format_type;
    short codec_type;
} AdxBasicDecoder;

typedef char AdxDecodeParamsSizeCheck[
    sizeof(AdxDecodeParams) == 0x2C ? 1 : -1];
typedef char AdxBasicDecoderSizeCheck[
    sizeof(AdxBasicDecoder) == 0xA0 ? 1 : -1];

typedef struct AhxDecoder AhxDecoder;
typedef void (*AdxDecodeNotify)(void*, int, int);

/* RE4's ADXB_OBJ confirms the +0xA0 keys and reserved +0xDA/+0xE0 spans;
 * MKD's decoder uses the same 0xF8 extent across ADXB and ADXSJD. */
typedef struct AdxBasicDecoderExt {
    AdxBasicDecoder base;
    short default_key[3];
    short snapshot_key[3];
    short delay_left[2];
    short delay_right[2];
    AhxDecoder* ahx_decoder;
    int ahx_max_decoded_samples;
    int ahx_max_decoded_blocks;
    int ainf_length;
    unsigned char ainf[16];
    short default_out_volume;
    short default_pan[2];
    unsigned char reserved_DA[2];
    void* pl2_context;
    unsigned char reserved_E0[8];
    int last_notified_data_length;
    int field_EC;
    AdxDecodeNotify notify;
    void* notify_object;
} AdxBasicDecoderExt;

typedef char AdxBasicDecoderExtSizeCheck[
    sizeof(AdxBasicDecoderExt) == 0xF8 ? 1 : -1];

int ADXB_GetFormat(AdxBasicDecoderExt*);
int ADXB_CheckSpsd(const signed char*);
void ADXB_ExecOneSpsd(AdxBasicDecoder*);
int ADXB_DecodeHeaderSpsd(AdxBasicDecoder*, signed char*, int);
int ADX_DecodeInfoSpsd(signed char*, int, short*, signed char*, signed char*, signed char*, signed char*, int*, int*, int*, short*);
#endif
