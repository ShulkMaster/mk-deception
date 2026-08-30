#include "cri/adx_basic.h"

int memcmp(const void*, const void*, unsigned long);

typedef struct WaveFormatEx {
    unsigned short format_tag;
    unsigned short channel_count;
    unsigned int samples_per_second;
    unsigned int average_bytes_per_second;
    unsigned short block_align;
    unsigned short bits_per_sample;
    unsigned short extra_size;
} WaveFormatEx;

typedef struct WaveChunk {
    unsigned char id[4];
    unsigned int size;
} WaveChunk;

static inline unsigned short ADXB_SwapWav16(unsigned short value)
{
    return (value & 0xFF) << 8 | value >> 8;
}

static inline unsigned int ADXB_SwapWav32(unsigned int value)
{
    return value >> 24 | (value >> 8 & 0xFF00) |
           (value << 8 & 0xFF0000) | value << 24;
}

void ADXB_ExecOneWav4(AdxBasicDecoder* decoder);
void ADXB_ExecOneWav8(AdxBasicDecoder* decoder);
void ADXB_ExecOneWav16(AdxBasicDecoder* decoder);
int ADX_DecodeInfoWav(signed char*, int, short*, signed char*, signed char*,
                      signed char*, signed char*, int*, int*, int*, short*);

void ADXB_ExecOneWav(AdxBasicDecoder* decoder)
{
    if (decoder->codec_type == 2) {
        ADXB_ExecOneWav4(decoder);
    } else if (decoder->codec_type == 1) {
        ADXB_ExecOneWav8(decoder);
    } else if (decoder->codec_type == 0) {
        ADXB_ExecOneWav16(decoder);
    }
}

int ADXB_CheckWav(const signed char* input)
{
    if (memcmp(input, "RIFF", 4) == 0 &&
        memcmp(&input[8], "WAVE", 4) == 0) {
        return 1;
    }

    return 0;
}

void ADXB_ExecOneWav4(AdxBasicDecoder* decoder)
{
    AdxDecodeParams* params;
    unsigned short* pcm;
    unsigned short* left;
    unsigned short* right;
    const unsigned char* input;
    int i;
    int count;

    params = &decoder->decode;
    input = (const unsigned char*)params->input;

    if (decoder->status == 1 && ADXPD_GetStat(decoder->expander) == 0) {
        decoder->get_write_info(decoder->get_write_object,
                                &params->write_position, &params->room,
                                &params->loop_samples);

        count = params->pcm_size - params->write_position;
        if (count > params->room) {
            count = params->room;
        }
        if (count > params->input_blocks) {
            count = params->input_blocks;
        }

        pcm = (unsigned short*)params->pcm_buffer;
        left = &pcm[params->write_position];

        if (decoder->channel_count == 2) {
            right = &pcm[params->pcm_distance + params->write_position];

            for (i = 0; i < count; i++) {
                left[i] = input[i * 4] | input[i * 4 + 2] * 256;
                right[i] = input[i * 4 + 1] | input[i * 4 + 3] * 256;
            }
        } else {
            for (i = 0; i < count; i++) {
                left[i] = input[i * 2] | input[i * 2 + 1] * 256;
            }
        }

        decoder->decoded_samples = count;
        decoder->decoded_data_length = count * 2 * decoder->channel_count;
        decoder->status = 2;
    }

    if (decoder->status == 2) {
        decoder->add_write_info(decoder->add_write_object,
                                decoder->decoded_data_length,
                                decoder->decoded_samples);
        decoder->status = 3;
    }
}

void ADXB_ExecOneWav8(AdxBasicDecoder* decoder)
{
    AdxDecodeParams* params;
    unsigned short* pcm;
    unsigned short* left;
    unsigned short* right;
    const unsigned char* input;
    int i;
    int count;

    params = &decoder->decode;
    input = (const unsigned char*)params->input;

    if (decoder->status == 1 && ADXPD_GetStat(decoder->expander) == 0) {
        decoder->get_write_info(decoder->get_write_object,
                                &params->write_position, &params->room,
                                &params->loop_samples);

        count = params->pcm_size - params->write_position;
        if (count > params->room) {
            count = params->room;
        }
        if (count > params->input_blocks) {
            count = params->input_blocks;
        }

        pcm = (unsigned short*)params->pcm_buffer;
        left = &pcm[params->write_position];

        if (decoder->channel_count == 2) {
            right = &pcm[params->pcm_distance + params->write_position];

            for (i = 0; i < count; i++) {
                left[i] = (input[i * 2] - 128) * 256;
                right[i] = (input[i * 2 + 1] - 128) * 256;
            }
        } else {
            for (i = 0; i < count; i++) {
                left[i] = (input[i] - 128) * 256;
            }
        }

        decoder->decoded_samples = count;
        decoder->decoded_data_length = count * decoder->channel_count;
        decoder->status = 2;
    }

    if (decoder->status == 2) {
        decoder->add_write_info(decoder->add_write_object,
                                decoder->decoded_data_length,
                                decoder->decoded_samples);
        decoder->status = 3;
    }
}

void ADXB_ExecOneWav16(AdxBasicDecoder* decoder)
{
    AdxDecodeParams* params;
    unsigned short* pcm;
    unsigned short* left;
    unsigned short* right;
    const unsigned short* input;
    unsigned short sample;
    int i;
    int count;

    params = &decoder->decode;
    input = params->input;

    if (decoder->status == 1 && ADXPD_GetStat(decoder->expander) == 0) {
        decoder->get_write_info(decoder->get_write_object,
                                &params->write_position, &params->room,
                                &params->loop_samples);

        count = params->pcm_size - params->write_position;
        if (count > params->room) {
            count = params->room;
        }
        if (count > params->input_blocks) {
            count = params->input_blocks;
        }

        pcm = (unsigned short*)params->pcm_buffer;
        left = &pcm[params->write_position];

        if (decoder->channel_count == 2) {
            right = &pcm[params->pcm_distance + params->write_position];

            for (i = 0; i < count; i++) {
                sample = input[i * 2];
                left[i] = sample << 8 | sample >> 8;
                sample = input[i * 2 + 1];
                right[i] = sample << 8 | sample >> 8;
            }
        } else {
            for (i = 0; i < count; i++) {
                sample = input[i];
                left[i] = sample << 8 | sample >> 8;
            }
        }

        decoder->decoded_samples = count;
        decoder->decoded_data_length = count * 2 * decoder->channel_count;
        decoder->status = 2;
    }

    if (decoder->status == 2) {
        decoder->add_write_info(decoder->add_write_object,
                                decoder->decoded_data_length,
                                decoder->decoded_samples);
        decoder->status = 3;
    }
}

int ADXB_DecodeHeaderWav(AdxBasicDecoder* decoder, signed char* input,
                         int input_length)
{
    short data_length;

    decoder->header_decoded = 1;

    if (ADX_DecodeInfoWav(input, input_length, &data_length,
                          &decoder->encoding, &decoder->bits_per_sample,
                          &decoder->block_length, &decoder->channel_count,
                          &decoder->sample_rate, &decoder->total_samples,
                          &decoder->samples_per_block,
                          &decoder->codec_type) < 0) {
        return 0;
    }

    decoder->coefficient = 0;
    decoder->loop_count = decoder->loop_type = 0;
    decoder->loop_insert_samples = decoder->loop_start_sample =
        decoder->loop_start_offset = decoder->loop_end_sample =
            decoder->loop_end_offset = 0;
    decoder->decode.channel_count = decoder->channel_count;
    decoder->decode.block_size = decoder->block_length;
    decoder->decode.samples_per_block = decoder->samples_per_block;
    decoder->decode.pcm_buffer = decoder->pcm_buffer;
    decoder->decode.pcm_size = decoder->pcm_size;
    decoder->decode.pcm_distance = decoder->pcm_distance;
    decoder->current_write_position = 0;
    decoder->total_decoded_samples = 0;
    decoder->format_type = 1;

    return data_length;
}

int ADX_DecodeInfoWav(signed char* input, int input_length,
                      short* data_length, signed char* encoding,
                      signed char* bits_per_sample, signed char* block_length,
                      signed char* channel_count, int* sample_rate,
                      int* total_samples, int* samples_per_block,
                      short* codec_type)
{
    WaveFormatEx* format;
    static signed char* fmt_id = (signed char*)"fmt ";
    static signed char* data_id = (signed char*)"data";
    WaveChunk* chunk;
    int i;
    int wav_size;

    for (i = 0; i < input_length; i++) {
        if (memcmp(&input[i], fmt_id, 4) == 0) {
            break;
        }
    }

    if (i == input_length) {
        return -1;
    }

    if (i % 4 != 0) {
        return -1;
    }

    format = (WaveFormatEx*)&input[i + 8];

    if ((short)ADXB_SwapWav16(format->format_tag) > 1) {
        return -1;
    }

    for (i = 0; i < input_length; i++) {
        if (memcmp(&input[i], data_id, 4) == 0) {
            break;
        }
    }

    if (i == input_length) {
        return -1;
    }

    chunk = (WaveChunk*)&input[i];
    wav_size = ADXB_SwapWav32(chunk->size);

    *data_length = i + 8;
    *encoding = -1;

    *sample_rate = ADXB_SwapWav32(format->samples_per_second);
    *channel_count = ADXB_SwapWav16(format->channel_count);
    *bits_per_sample = ADXB_SwapWav16(format->bits_per_sample);
    *block_length = ADXB_SwapWav16(format->block_align);
    *total_samples = wav_size / *block_length;
    *samples_per_block = 1;

    if (*bits_per_sample == 16) {
        *codec_type = 0;
    } else if (*bits_per_sample == 8) {
        *codec_type = 1;
    } else if (*bits_per_sample == 4) {
        *block_length = *channel_count * 2;
        *samples_per_block = 4;
        *total_samples = (wav_size / 2) / *channel_count;
        *bits_per_sample = 16;
        *codec_type = 2;
    }

    if (*bits_per_sample == 0) {
        return -1;
    }

    if (*block_length == 0) {
        return -1;
    }

    if (*channel_count <= 0 || *channel_count > 2) {
        return -1;
    }

    return *sample_rate == 0 ? -1 : 0;
}
