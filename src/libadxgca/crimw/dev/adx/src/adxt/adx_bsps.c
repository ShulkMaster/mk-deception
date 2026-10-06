#include "cri/adx_basic.h"
#include "runtime/cstring.h"

struct SpsdHeaderView {
    unsigned char pad00[7];
    unsigned char field_0x07;
    unsigned char field_0x08;
    unsigned char channel_flags;
    unsigned char pad0A[2];
    int sample_data_bytes;
    unsigned char pad10[0x1A];
    unsigned short sample_rate;
};

int ADXB_CheckSpsd(const signed char* input) { return memcmp(input, "SPSD", 4) == 0; }

void ADXB_ExecOneSpsd(AdxBasicDecoder* decoder)
{
    AdxDecodeParams* params;
    short* pcm;
    short* left;
    short* right;
    const unsigned short* input;
    int i;
    int count;

    params = &decoder->decode;
    input = params->input;
    if (decoder->status == 1 && ADXPD_GetStat(decoder->expander) == 0) {
        decoder->get_write_info(decoder->get_write_object, &params->write_position,
                                &params->room, &params->loop_samples);
        count = params->pcm_size - params->write_position;
        if (count > params->room) count = params->room;
        if (count > params->input_blocks) count = params->input_blocks;
        pcm = params->pcm_buffer;
        left = &pcm[params->write_position];
        if (decoder->channel_count == 2) {
            right = &pcm[params->pcm_distance + params->write_position];
            for (i = 0; i < count; i++) {
                left[i] = input[i * 2];
                right[i] = input[i * 2 + 1];
            }
        } else {
            for (i = 0; i < count; i++) left[i] = input[i];
        }
        decoder->decoded_samples = count;
        decoder->decoded_data_length = decoder->channel_count * (count << 1);
        decoder->status = 2;
    }
    if (decoder->status == 2) {
        decoder->add_write_info(decoder->add_write_object,
                                decoder->decoded_data_length, decoder->decoded_samples);
        decoder->status = 3;
    }
}

int ADXB_DecodeHeaderSpsd(AdxBasicDecoder* decoder, signed char* input, int input_length)
{
    short data_length;
    decoder->header_decoded = 1;
    if (ADX_DecodeInfoSpsd(input, input_length, &data_length, &decoder->encoding,
                           &decoder->bits_per_sample, &decoder->block_length,
                           &decoder->channel_count, &decoder->sample_rate,
                           &decoder->total_samples, &decoder->samples_per_block,
                           &decoder->codec_type) < 0)
        return 0;
    decoder->coefficient = 0;
    decoder->loop_type = 0;
    decoder->loop_count = 0;
    decoder->loop_end_offset = 0;
    decoder->loop_end_sample = 0;
    decoder->loop_start_offset = 0;
    decoder->loop_start_sample = 0;
    decoder->loop_insert_samples = 0;
    decoder->decode.channel_count = decoder->channel_count;
    decoder->decode.block_size = decoder->block_length;
    decoder->decode.samples_per_block = decoder->samples_per_block;
    decoder->decode.pcm_buffer = decoder->pcm_buffer;
    decoder->decode.pcm_size = decoder->pcm_size;
    decoder->decode.pcm_distance = decoder->pcm_distance;
    decoder->current_write_position = 0;
    decoder->total_decoded_samples = 0;
    decoder->format_type = 2;
    return data_length;
}

int ADX_DecodeInfoSpsd(signed char* input, int input_length, short* data_length,
                       signed char* encoding, signed char* bits_per_sample,
                       signed char* block_length, signed char* channel_count,
                       int* sample_rate, int* total_samples,
                       int* samples_per_block, short* codec_type)
{
    const struct SpsdHeaderView* header = (const struct SpsdHeaderView*)input;
    unsigned char bit_size;
    *data_length = header->field_0x07 * 16;
    *channel_count = (header->channel_flags & 3) + 1;
    *sample_rate = header->sample_rate;
    bit_size = header->field_0x08;
    switch (bit_size) {
    case 0:
        *bits_per_sample = 16;
        *block_length = *channel_count * 2;
        *samples_per_block = 1;
        *total_samples = header->sample_data_bytes / 2;
        *codec_type = 0;
        break;
    case 1:
        *bits_per_sample = 8;
        *block_length = *channel_count;
        *samples_per_block = 1;
        *total_samples = header->sample_data_bytes;
        *codec_type = 1;
        break;
    case 2:
    case 3:
        *bits_per_sample = 4;
        *block_length = *channel_count;
        *samples_per_block = 2;
        *total_samples = header->sample_data_bytes * 2;
        *codec_type = 2;
        break;
    }
    *block_length = 2;
    *samples_per_block = 1;
    *total_samples = header->sample_data_bytes / 2;
    *bits_per_sample = 16;
    *encoding = -1;
    return 0;
}
