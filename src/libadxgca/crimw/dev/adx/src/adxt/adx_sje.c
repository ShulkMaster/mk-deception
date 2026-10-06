#include "cri/adx_dcd.h"
#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

enum {
    ADXSJE_MAX_HANDLES = 8,
    ADXSJE_MAX_CHANNELS = 2,
    ADXSJE_MAX_FILTERS = 16,
    ADXSJE_MAX_BLOCK_SAMPLES = 32,
    ADXSJE_BLOCK_BYTES = 18
};

struct AdxSjeIirFilter {
    s8 used;
    u8 reserved_01[3];
    s16 coefficient0;
    s16 coefficient1;
    s16 previous0;
    s16 previous1;
};

struct AdxSjePredictorFilter {
    s8 used;
    u8 reserved_01[3];
    s16 coefficient0;
    s16 coefficient1;
    s16 previous0;
    s16 previous1;
    u8 reserved_0C[8];
    s32 sample_count;
    s16 residual[ADXSJE_MAX_BLOCK_SAMPLES];
    s8 code[ADXSJE_MAX_BLOCK_SAMPLES];
    s32 maximum;
    s16 scale;
    u8 reserved_7E[2];
    f64 gain;
    struct AdxSjeIirFilter* iir_filter;
};

typedef struct AdxSjeHandle {
    s8 used;
    s8 status;
    s8 input_count;
    s8 stopped;
    SJ* input[ADXSJE_MAX_CHANNELS];
    SJ* output;
    u8 reserved_10[0x18];
    s32 data_length;
    s32 output_length;
    s32 sample_position;
    s32 maximum_length;
    s32 field_38;
    s32 sample_count;
    s32 padded_sample_count;
    s32 block_samples;
    s32 header_length;
    s32 encoding_type;
    s32 block_size;
    s32 bits_per_sample;
    s32 channel_count;
    s32 sample_rate;
    s32 total_samples;
    s32 cutoff_frequency;
    s32 alignment_samples;
    s32 loop_count;
    s32 loop_start_sample;
    s32 loop_start_offset;
    s32 loop_end_sample;
    s32 loop_end_offset;
    struct AdxSjePredictorFilter* filter[ADXSJE_MAX_CHANNELS];
    s16 previous0[ADXSJE_MAX_CHANNELS];
    s16 previous1[ADXSJE_MAX_CHANNELS];
    s16 samples[ADXSJE_MAX_CHANNELS][ADXSJE_MAX_BLOCK_SAMPLES];
    s16 residual[ADXSJE_MAX_CHANNELS][ADXSJE_MAX_BLOCK_SAMPLES];
    s16 scaled[ADXSJE_MAX_CHANNELS][ADXSJE_MAX_BLOCK_SAMPLES];
    s16 reconstructed[ADXSJE_MAX_CHANNELS][ADXSJE_MAX_BLOCK_SAMPLES];
    s16 scale[ADXSJE_MAX_CHANNELS];
    u8 reserved_294[4];
    f64 gain[ADXSJE_MAX_CHANNELS];
    u8 encoded[ADXSJE_MAX_CHANNELS][16];
    s16 initial_previous0[ADXSJE_MAX_CHANNELS];
    s16 initial_previous1[ADXSJE_MAX_CHANNELS];
    s16 random_seed;
    s16 random_multiplier;
    s16 random_increment;
    s8 ainf_enabled;
    u8 ainf[16];
    u8 reserved_2E7;
    s16 ainf_front;
    s16 ainf_center;
    s16 ainf_surround;
    s8 cinf_enabled;
    u8 reserved_2EF;
    u8* cinf;
    s32 cinf_length;
} AdxSjeHandle;

typedef char AdxSjeIirFilterSizeCheck[
    sizeof(struct AdxSjeIirFilter) == 0x0C ? 1 : -1];
typedef char AdxSjePredictorFilterSizeCheck[
    sizeof(struct AdxSjePredictorFilter) == 0x90 ? 1 : -1];
typedef char AdxSjeHandleSizeCheck[
    sizeof(AdxSjeHandle) == 0x2F8 ? 1 : -1];

extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);

static s32 AdxGainDataMax = 0x1000;
static const char* cri_str = "(c)CRI";

static s32 skg_init_count = 0;
static void (*skg_err_func)(void*) = 0;
static void* skg_err_obj = 0;
struct AdxSjePredictorFilter adxsje_prdflt_obj[ADXSJE_MAX_FILTERS];
struct AdxSjeIirFilter adxsje_iirflt_obj[ADXSJE_MAX_FILTERS];
AdxSjeHandle adxsje_obj[ADXSJE_MAX_HANDLES];

static void adxsje_flt_init(void)
{
    memset(adxsje_prdflt_obj, 0, sizeof(adxsje_prdflt_obj));
    memset(adxsje_iirflt_obj, 0, sizeof(adxsje_iirflt_obj));
}

void ADXSJE_ExecHndl(AdxSjeHandle* encoder);
void adxsje_encode_exec(AdxSjeHandle* encoder);
s32 adxsje_output_header(AdxSjeHandle* encoder, SJ* output);
s32 adxsje_write_end_code(AdxSjeHandle* encoder);
s32 adxsje_encode_data(AdxSjeHandle* encoder);
s32 adxsje_output_sdata(AdxSjeHandle* encoder);
void adxsje_set_rsig(AdxSjeHandle* encoder, s32 channel);
s32 adxsje_calc_rsig(AdxSjeHandle* encoder, s32 channel);
s32 adxsje_write68(const void* source, s32 element_size, s32 count, SJ* output);

static inline s32 adxsje_clamp_s16_range(s32 value)
{
    return (value < -0x8000 ? -0x8000 : value) > 0x7FFF
               ? 0x7FFF
               : (value < -0x8000 ? -0x8000 : value);
}

static inline s16 adxsje_clamp_to_s16(s32 value)
{
    return adxsje_clamp_s16_range(value);
}

static inline s32 adxsje_clamp_code(s32 value)
{
    return (value < -8 ? -8 : value) < 7
               ? (value < -8 ? -8 : value)
               : 7;
}

static inline struct AdxSjeIirFilter* adxsje_alloc_iir(void)
{
    s32 index;
    struct AdxSjeIirFilter* filter;

    for (index = 0; index < ADXSJE_MAX_FILTERS; index++) {
        filter = &adxsje_iirflt_obj[index];
        if (filter->used == 0) {
            break;
        }
    }
    return index < ADXSJE_MAX_FILTERS ? filter : 0;
}

static inline struct AdxSjePredictorFilter* adxsje_create_filter(s32 sample_count)
{
    struct AdxSjePredictorFilter* filter;
    struct AdxSjeIirFilter* iir_filter;
    s32 index;

    iir_filter = adxsje_alloc_iir();
    if (iir_filter == 0) {
        return 0;
    }
    for (index = 0; index < ADXSJE_MAX_FILTERS; index++) {
        filter = &adxsje_prdflt_obj[index];
        if (filter->used == 0) {
            break;
        }
    }
    filter->sample_count = sample_count;
    filter->iir_filter = iir_filter;
    if (index < ADXSJE_MAX_FILTERS) {
        filter->used = 1;
    } else {
        filter = 0;
    }
    return filter;
}

static inline void adxsje_destroy_filter(struct AdxSjePredictorFilter* filter)
{
    if (filter != 0) {
        filter->used = 0;
        memset(filter, 0, sizeof(*filter));
    }
}

static inline void adxsje_write_chunk(SJ* output, const void* data, s32 size)
{
    SJCK chunk;
    output->interface->get_chunk(output, 0, size, &chunk);
    if (chunk.len < size) {
        output->interface->unget_chunk(output, 0, &chunk);
    } else {
        if (size == (s32)sizeof(s16)) {
            *(s16*)chunk.data = *(const s16*)data;
        } else {
            memcpy(chunk.data, data, size);
        }
        output->interface->put_chunk(output, 1, &chunk);
    }
}

void ADXSJE_ExecServer(void)
{
    s32 index;

    for (index = 0; index < ADXSJE_MAX_HANDLES; index++) {
        if (adxsje_obj[index].used == 1) {
            ADXSJE_ExecHndl(&adxsje_obj[index]);
        }
    }
}

static inline void adxsje_prdflt_set_coef(struct AdxSjePredictorFilter* filter,
                                           s16 coefficient0, s16 coefficient1)
{
    filter->coefficient0 = coefficient0;
    filter->coefficient1 = coefficient1;
}

static inline void adxsje_iirflt_set_coef(struct AdxSjeIirFilter* filter,
                                           s16 coefficient0, s16 coefficient1)
{
    filter->coefficient0 = coefficient0;
    filter->coefficient1 = coefficient1;
}

static inline void adxsje_initialize_filter_coefficients(AdxSjeHandle* encoder,
                                                        struct AdxSjePredictorFilter* filter)
{
    s16 coefficient0;
    s16 coefficient1;

    ADX_GetCoefficient((s16)encoder->cutoff_frequency,
                       encoder->sample_rate, &coefficient0, &coefficient1);
    adxsje_prdflt_set_coef(filter, coefficient0, coefficient1);
    adxsje_iirflt_set_coef(filter->iir_filter, coefficient0, coefficient1);
}

static inline void adxsje_header_exec(AdxSjeHandle* encoder)
{
    SJ* output;
    s32 channel;
    s32 header_size;
    SJCK chunk;
    s16 sample;

    output = encoder->output;
    for (channel = 0; channel < encoder->channel_count; channel++) {
        encoder->input[channel]->interface->get_chunk(
            encoder->input[channel], 1, 2, &chunk);
        if (chunk.len == 0) {
            break;
        }
        sample = *(s16*)chunk.data;
        encoder->initial_previous1[channel] = sample;
        encoder->initial_previous0[channel] = sample;
        encoder->input[channel]->interface->unget_chunk(
            encoder->input[channel], 1, &chunk);
    }
    if (channel < encoder->channel_count) {
        return;
    }
    for (channel = 0; channel < encoder->channel_count; channel++) {
        encoder->previous0[channel] = encoder->initial_previous0[channel];
        encoder->previous1[channel] = encoder->initial_previous1[channel];
    }
    header_size = adxsje_output_header(encoder, output);
    if (header_size == 0) {
        return;
    }
    encoder->output_length += header_size;
    for (channel = 0; channel < encoder->channel_count; channel++) {
        adxsje_initialize_filter_coefficients(encoder, encoder->filter[channel]);
    }
    encoder->status = 2;
}

void ADXSJE_ExecHndl(AdxSjeHandle* encoder)
{
    if (encoder->status == 1) {
        adxsje_header_exec(encoder);
    } else if (encoder->status == 2) {
        adxsje_encode_exec(encoder);
    }
}

void adxsje_encode_exec(AdxSjeHandle* encoder)
{
    s32 size;

    for (;;) {
        if (encoder->stopped != 0) {
            break;
        }
        do {
            size = adxsje_encode_data(encoder);
            if (size == 0) {
                return;
            }
            encoder->output_length += size;
        } while (encoder->sample_position < encoder->sample_count);
        encoder->stopped = 1;
    }
    if (adxsje_write_end_code(encoder) > 0) {
        encoder->status = 3;
    }
}

void ADXSJE_SetConfigSfa(AdxSjeHandle* encoder, s32 channels,
                         s32 sample_rate, s32 sample_count)
{
    encoder->channel_count = channels;
    encoder->sample_rate = sample_rate;
    encoder->sample_count = sample_count;
    encoder->total_samples = sample_count;
    encoder->header_length = 0x11C;
}

void ADXSJE_Stop(AdxSjeHandle* encoder)
{
    encoder->stopped = 1;
}

void ADXSJE_Start(AdxSjeHandle* encoder)
{
    s32 channel;
    s32 byte_count;
    SJCK chunk;

    for (channel = 0; channel < encoder->channel_count; channel++) {
        byte_count = encoder->alignment_samples * 2;
        if (byte_count > 0) {
            encoder->input[channel]->interface->get_chunk(
                encoder->input[channel], 0, byte_count, &chunk);
            if (chunk.len != byte_count) {
                encoder->input[channel]->interface->unget_chunk(
                    encoder->input[channel], 0, &chunk);
                for (;;) {
                }
            }
            memset(chunk.data, 0, chunk.len);
            encoder->input[channel]->interface->put_chunk(
                encoder->input[channel], 1, &chunk);
        }
    }
    encoder->data_length = 0;
    encoder->output_length = 0;
    encoder->sample_position = 0;
    encoder->stopped = 0;
    encoder->status = 1;
}

void ADXSJE_Destroy(AdxSjeHandle* encoder)
{
    if (encoder != 0) {
        ADXCRS_Lock();
        if (encoder->filter[0] != 0) {
            adxsje_destroy_filter(encoder->filter[0]);
        }
        if (encoder->filter[1] != 0) {
            adxsje_destroy_filter(encoder->filter[1]);
        }
        memset(encoder, 0, 4);
        ADXCRS_Unlock();
    }
}

AdxSjeHandle* ADXSJE_Create(s32 input_count, SJ** input, SJ* output)
{
    s32 index;
    AdxSjeHandle* encoder;

    for (index = 0; index < ADXSJE_MAX_HANDLES; index++) {
        if (adxsje_obj[index].used == 0) {
            break;
        }
    }
    if (index == ADXSJE_MAX_HANDLES) {
        return 0;
    }
    encoder = &adxsje_obj[index];
    encoder->input_count = input_count;
    for (index = 0; index < input_count; index++) {
        encoder->input[index] = input[index];
    }
    encoder->output = output;
    encoder->status = 0;
    encoder->data_length = 0;
    encoder->output_length = 0;
    encoder->sample_position = 0;
    encoder->maximum_length = 0x7FFF0000;
    encoder->header_length = ADX_CalcHdrInfoLen(0, 0, 4, 4);
    encoder->encoding_type = 3;
    encoder->channel_count = input_count;
    encoder->sample_rate = 44100;
    encoder->bits_per_sample = 4;
    encoder->block_size = ADXSJE_BLOCK_BYTES;
    encoder->block_samples =
        ((encoder->block_size - 2) * 8) / encoder->bits_per_sample;
    encoder->total_samples = 0x7FFF0000;
    encoder->cutoff_frequency = 500;
    encoder->alignment_samples = 0;
    encoder->loop_count = 0;
    encoder->loop_start_sample = 0;
    encoder->loop_start_offset = 0;
    encoder->loop_end_sample = 0;
    encoder->loop_end_offset = 0;
    encoder->field_38 = 0;
    encoder->sample_count = 0;
    encoder->padded_sample_count = encoder->block_samples *
        (((encoder->total_samples - 1) / encoder->block_samples) + 1);
    encoder->filter[0] = adxsje_create_filter(encoder->block_samples);
    encoder->filter[1] = adxsje_create_filter(encoder->block_samples);
    encoder->initial_previous1[0] = 0;
    encoder->initial_previous0[0] = 0;
    encoder->initial_previous1[1] = 0;
    encoder->initial_previous0[1] = 0;
    encoder->ainf_enabled = 0;
    encoder->ainf_front = 0;
    encoder->ainf_center = -0x80;
    encoder->ainf_surround = -0x80;
    memset(encoder->ainf, 0, sizeof(encoder->ainf));
    encoder->used = 1;
    return encoder;
}

void ADXSJE_Finish(void)
{
    skg_init_count--;
    memset(adxsje_obj, 0, sizeof(adxsje_obj));
}

void ADXSJE_Init(void)
{
    skg_init_count++;
    memset(adxsje_obj, 0, sizeof(adxsje_obj));
}

s32 adxsje_output_header(AdxSjeHandle* encoder, SJ* output)
{
    s32 signature_length;
    s32 written;
    s32 limit;
    s32 value32;
    u16 signature;
    s16 value16;
    s16 header;
    s8 value8;
    s32 index;
    SJCK chunk;

    signature_length = strlen(cri_str);
    output->interface->get_chunk(output, 0, 0x7FFFFFFF, &chunk);
    limit = chunk.len;
    output->interface->unget_chunk(output, 0, &chunk);
    if (limit < encoder->header_length + 4) {
        return 0;
    }

    signature = 0x8000;
    if (adxsje_write68(&signature, 2, 1, output) != 1) goto fail;
    header = encoder->header_length;
    if (adxsje_write68(&header, 2, 1, output) != 1) goto fail;
    value8 = encoder->encoding_type;
    if (adxsje_write68(&value8, 1, 1, output) != 1) goto fail;
    value8 = encoder->block_size;
    if (adxsje_write68(&value8, 1, 1, output) != 1) goto fail;
    value8 = encoder->bits_per_sample;
    if (adxsje_write68(&value8, 1, 1, output) != 1) goto fail;
    value8 = encoder->channel_count;
    if (adxsje_write68(&value8, 1, 1, output) != 1) goto fail;
    value32 = encoder->sample_rate;
    if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
    value32 = encoder->total_samples;
    if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
    value16 = encoder->cutoff_frequency;
    if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
    value8 = 4;
    if (adxsje_write68(&value8, 1, 1, output) != 1) goto fail;
    if (encoder->random_seed == 0) {
        value8 = 0;
    } else {
        value8 = 8;
    }
    if (adxsje_write68(&value8, 1, 1, output) != 1) goto fail;
    value32 = 0;
    if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
    value16 = encoder->initial_previous0[0];
    if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
    value16 = encoder->initial_previous1[0];
    if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
    value16 = encoder->initial_previous0[1];
    if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
    value16 = encoder->initial_previous1[1];
    if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;

    written = 0x1C;
    if (encoder->loop_count > 0) {
        value16 = encoder->alignment_samples;
        if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
        value16 = encoder->loop_count;
        if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
        index = 0;
        written = 0x20;
        for (; index < encoder->loop_count; index++) {
            value16 = index;
            if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
            value16 = 1;
            if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
            value32 = encoder->loop_start_sample;
            if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
            value32 = encoder->loop_start_offset;
            if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
            value32 = encoder->loop_end_sample;
            if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
            value32 = encoder->loop_end_offset;
            if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
            written += 0x14;
        }
    }

    if (encoder->ainf_enabled == 1) {
        if (encoder->loop_count == 0) {
            value32 = 0;
            if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
            written += 4;
        }
        value32 = 0x41494E46;
        if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
        value32 = 0x18;
        if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
        if (adxsje_write68(encoder->ainf, 1, sizeof(encoder->ainf), output) !=
            sizeof(encoder->ainf)) goto fail;
        value16 = encoder->ainf_front;
        if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
        value16 = 0;
        if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
        value16 = encoder->ainf_center;
        if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
        value16 = encoder->ainf_surround;
        if (adxsje_write68(&value16, 2, 1, output) != 1) goto fail;
        written += 0x20;
    }

    if (encoder->cinf_enabled == 1) {
        if (encoder->loop_count == 0) {
            value32 = 0;
            if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
            written += 4;
        }
        value32 = 0x43494E46;
        if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
        value32 = encoder->cinf_length;
        if (adxsje_write68(&value32, 4, 1, output) != 1) goto fail;
        written += 8;
        if (encoder->cinf_length != 0 && encoder->cinf != 0) {
            if (adxsje_write68(encoder->cinf, 1, encoder->cinf_length,
                               output) != encoder->cinf_length) {
                goto fail;
            }
            written += encoder->cinf_length;
        }
    }

    value8 = 0;
    limit = encoder->header_length - signature_length;
    while (written < limit) {
        if (adxsje_write68(&value8, 1, 1, output) != 1) {
            goto fail;
        }
        written++;
    }
    if (adxsje_write68(cri_str, 1, signature_length, output) !=
        signature_length) {
        goto fail;
    }
    written += signature_length;
    return written + 4;

fail:
    return 0;
}

/* TODO: [near miss] 97.14%; marker stack/widths agree; first native-word destination/source loads reordered. */
s32 adxsje_write_end_code(AdxSjeHandle* encoder)
{
    s32 end_size;
    s32 output_size;
    SJ* output;
    s32 output_length;
    s32 index;
    u16 value;
    u8 zero;

    output = encoder->output;
    output_length = encoder->output_length;
    if (encoder->loop_count <= 0) {
        output_size = encoder->block_size;
    } else {
        output_size =
            ((output_length + encoder->block_size + 0x7FF) / 0x800) * 0x800 -
            output_length;
    }
    end_size = output_size - 4;
    if (output->interface->get_num_data(output, 0) < output_size) {
        return 0;
    }
    value = 0x8001;
    adxsje_write_chunk(output, &value, sizeof(value));
    value = end_size;
    adxsje_write_chunk(output, &value, sizeof(value));
    zero = 0;
    for (index = 0; index < end_size; index++) {
        adxsje_write_chunk(output, &zero, sizeof(zero));
    }
    return output_size;
}

static inline s32 adxsje_read_pcm(AdxSjeHandle* encoder, SJ** inputs,
                                  s16** sample_buffer, s32 requested_samples)
{
    SJ** input_cursor;
    s32 pcm_samples;
    s32 channel;
    s32 byte_count;
    SJCK chunk;

    input_cursor = inputs;
    pcm_samples = 0;
    for (channel = 0; channel < encoder->channel_count; input_cursor++, channel++) {
        pcm_samples = (u32)(*input_cursor)->interface->get_num_data(
            *input_cursor, 1) / sizeof(s16);
        if (pcm_samples < requested_samples) {
            return 0;
        }
    }
    byte_count = requested_samples * (s32)sizeof(s16);
    input_cursor = inputs;
    for (channel = 0; channel < encoder->channel_count; input_cursor++, channel++) {
        pcm_samples = 0;
        while (pcm_samples < requested_samples) {
            (*input_cursor)->interface->get_chunk(
                *input_cursor, 1, byte_count, &chunk);
            memcpy(sample_buffer[channel] + pcm_samples, chunk.data, chunk.len);
            pcm_samples += (s16)((u32)chunk.len / sizeof(s16));
            (*input_cursor)->interface->put_chunk(*input_cursor, 0, &chunk);
        }
    }
    return pcm_samples;
}

static inline s32 adxsje_encode_pcm_block(AdxSjeHandle* encoder)
{
    SJ** inputs;
    s32 requested_samples;
    s32 pcm_samples;
    s16* sample_buffer[ADXSJE_MAX_CHANNELS];

    requested_samples = encoder->sample_count - encoder->sample_position;
    sample_buffer[0] = encoder->samples[0];
    sample_buffer[1] = encoder->samples[1];
    inputs = encoder->input;
    if (encoder->block_samples < requested_samples) {
        requested_samples = encoder->block_samples;
    }
    pcm_samples = adxsje_read_pcm(encoder, inputs, sample_buffer, requested_samples);
    if (pcm_samples == 0) {
        return 0;
    } else {
        s32 channel;
        if (requested_samples < encoder->block_samples) {
            s32 padding_channel;
            for (padding_channel = 0; padding_channel < encoder->channel_count; padding_channel++) {
                if (sample_buffer[padding_channel] != 0) {
                    memset(sample_buffer[padding_channel] + requested_samples, 0,
                           (encoder->block_samples - requested_samples) * (s32)sizeof(s16));
                }
            }
        }

        encoder->sample_position += encoder->block_samples;
        for (channel = 0; channel < encoder->channel_count; channel++) {
            struct AdxSjePredictorFilter* filter;
            struct AdxSjeIirFilter* iir_filter;
            s16 previous1;

            adxsje_calc_rsig(encoder, channel);
            filter = encoder->filter[channel];
            iir_filter = filter->iir_filter;
            encoder->scale[channel] = filter->scale;
            encoder->gain[channel] = filter->gain;
            previous1 = iir_filter->previous1;
            encoder->previous0[channel] = iir_filter->previous0;
            encoder->previous1[channel] = previous1;
            adxsje_set_rsig(encoder, channel);
        }
        return encoder->block_samples;
    }
}

/* TODO: [near miss] 98.68263%; PCM transfer and filter-history register assignments remain. */
s32 adxsje_encode_data(AdxSjeHandle* encoder)
{
    s32 total_size;
    s32 produced;
    s32 encoded_samples;
    SJ* output;

    total_size = 0;
    output = encoder->output;
    for (;;) {
        if ((output->interface->get_num_data(output, 0) /
             ADXSJE_BLOCK_BYTES) / encoder->channel_count <= 0) {
            break;
        }
        encoded_samples = adxsje_encode_pcm_block(encoder);
        if (encoded_samples == 0) {
            break;
        }
        produced = adxsje_output_sdata(encoder);
        total_size += produced;
        if (encoder->sample_position >= encoder->sample_count) {
            break;
        }
    }
    return total_size;
}

s32 adxsje_output_sdata(AdxSjeHandle* encoder)
{
    s32 channel;
    s32 total_size;
    SJ* output;
    s32 value;
    s16 sample;
    s8 byte;

    channel = 0;
    total_size = 0;
    output = encoder->output;
    for (; channel < encoder->channel_count; channel++) {
        const s32* encoded_words;

        value = (s16)(encoder->scale[channel] - 1) ^ encoder->random_seed;
        encoder->random_seed = (s16)(encoder->random_increment +
            encoder->random_seed * encoder->random_multiplier);
        encoder->random_seed &= 0x7FFF;
        encoded_words = (const s32*)encoder->encoded[channel];
        if (encoded_words[0] == 0 && encoded_words[1] == 0 &&
            encoded_words[2] == 0 && encoded_words[3] == 0) {
            value = 0;
        }
        sample = value;
        byte = sample >> 8;
        adxsje_write_chunk(output, &byte, 1);
        byte = sample;
        adxsje_write_chunk(output, &byte, 1);
        adxsje_write_chunk(output, encoder->encoded[channel], sizeof(encoder->encoded[channel]));
        total_size += ADXSJE_BLOCK_BYTES;
    }
    return total_size;
}

void adxsje_set_rsig(AdxSjeHandle* encoder, s32 channel)
{
    s32 index;
    s32 code;
    s32 bit_index;
    s32 samples_per_byte;
    s32 output_index;
    s32 scaled_value;
    s32 reconstructed_value;
    s16 sample;
    s32 clipped;
    struct AdxSjePredictorFilter* filter;
    s16* residual;
    s16* scaled;
    s16* reconstructed;
    u8* encoded;

    residual = encoder->residual[channel];
    scaled = encoder->scaled[channel];
    reconstructed = encoder->reconstructed[channel];
    encoded = encoder->encoded[channel];
    output_index = -1;
    samples_per_byte = 8 / encoder->bits_per_sample;
    filter = encoder->filter[channel];
    bit_index = 0;
    for (index = 0; index < encoder->block_samples; index++) {
        s32 code_shift;
        sample = filter == 0 ? 0 : filter->residual[index];
        residual[index] = sample;
        scaled_value = (s32)(filter->gain * residual[index]);
        clipped = adxsje_clamp_s16_range(scaled_value);
        scaled[index] = clipped;
        if (scaled[index] < 0) {
            code = (clipped - 0x924) / 4681;
        } else {
            code = (clipped + 0x924) / 4681;
        }
        code = adxsje_clamp_code(code);
        reconstructed_value = (s32)(filter->gain * (code * filter->scale));
        reconstructed[index] = adxsje_clamp_s16_range(reconstructed_value);

        if ((index % samples_per_byte) == 0) {
            bit_index = 1;
            encoded[++output_index] = 0;
        }
        code_shift = encoder->bits_per_sample;
        code_shift = 8 - code_shift;
        code = (u8)((u32)code << code_shift) >> code_shift;
        encoded[output_index] |= (s8)(code <<
            (encoder->bits_per_sample * (samples_per_byte - bit_index)));
        bit_index++;
    }
}

static inline void adxsje_prdflt_set_hist(
    struct AdxSjePredictorFilter* filter, s16 previous0, s16 previous1)
{
    filter->previous0 = previous0;
    filter->previous1 = previous1;
}

static inline void adxsje_prdflt_exec(
    struct AdxSjePredictorFilter* filter, s16 sample, s32 index)
{
    s32 value;
    s16 residual;
    s32 magnitude;

    if (index == 0) {
        s32 clear_index;

        for (clear_index = 0; clear_index < filter->sample_count;
             clear_index++) {
            filter->residual[clear_index] = 0;
        }
        filter->maximum = 0;
    }
    if (filter != 0) {
        value = sample -
            ((filter->coefficient0 * filter->previous0) >> 12) -
            ((filter->coefficient1 * filter->previous1) >> 12);
        residual = adxsje_clamp_to_s16(value);
        filter->residual[index] = residual;
        magnitude = residual < 0 ? -residual : residual;
        if (magnitude > filter->maximum) {
            filter->maximum = residual < 0 ? -residual : residual;
        }
        filter->previous1 = filter->previous0;
        filter->previous0 = sample;
    }
}

static inline void adxsje_iirflt_set_hist(
    struct AdxSjeIirFilter* filter, s16 previous0, s16 previous1)
{
    filter->previous0 = previous0;
    filter->previous1 = previous1;
}

static inline s16 adxsje_clamp_iir_s16(s32 value)
{
    return (s16)(((value > -0x8000 ? value : -0x8000) < 0x7FFF)
                     ? (value > -0x8000 ? value : -0x8000)
                     : 0x7FFF);
}

static inline void adxsje_iirflt_exec(struct AdxSjeIirFilter* filter, s32 sample)
{
    s32 value;
    s16 output_sample;

    if (filter == 0) {
        return;
    }
    value = sample +
        ((filter->coefficient0 * filter->previous0 +
          filter->coefficient1 * filter->previous1) >> 12);
    output_sample = adxsje_clamp_iir_s16(value);
    filter->previous1 = filter->previous0;
    filter->previous0 = output_sample;
}

s32 adxsje_calc_rsig(AdxSjeHandle* encoder, s32 channel)
{
    s16 sample;
    s16* samples;
    s32 index;
    s32 value;
    s32 code;
    s32 residual;
    s16 clipped;
    s16 initial_previous0;
    s16 initial_previous1;
    struct AdxSjePredictorFilter* filter;
    struct AdxSjeIirFilter* iir_filter;

    initial_previous0 = encoder->previous0[channel];
    initial_previous1 = encoder->previous1[channel];
    filter = encoder->filter[channel];
    iir_filter = filter->iir_filter;
    adxsje_prdflt_set_hist(
        filter, initial_previous0, initial_previous1);
    samples = encoder->samples[channel];
    for (index = 0; index < encoder->block_samples; index++) {
        sample = samples[index];
        adxsje_prdflt_exec(filter, sample, index);
    }

    value = ((filter->maximum - 1) / 7) + 1;
    value = (value < AdxGainDataMax ? value : AdxGainDataMax) > 1
                ? (value < AdxGainDataMax ? value : AdxGainDataMax)
                : 1;
    filter->scale = value;
    if (filter->maximum == 0) {
        filter->gain = 32767.0;
    } else {
        filter->gain = 32767.0 / filter->maximum;
    }

    adxsje_iirflt_set_hist(
        filter->iir_filter, encoder->previous0[channel], encoder->previous1[channel]);
    samples = encoder->samples[channel];
    for (index = 0; index < encoder->block_samples; samples++, index++) {
        sample = *samples;
        adxsje_prdflt_set_hist(
            filter, iir_filter->previous0, iir_filter->previous1);
        adxsje_prdflt_exec(filter, sample, index);

        residual = filter == 0 ? 0 : filter->residual[index];
        value = (s32)((s16)residual * filter->gain);
        clipped = adxsje_clamp_to_s16(value);
        if (clipped < 0) {
            code = (clipped - 0x924) / 4681;
        } else {
            code = (clipped + 0x924) / 4681;
        }
        code = adxsje_clamp_code(code);
        filter->code[index] = code;
        adxsje_iirflt_exec(filter->iir_filter,
                           adxsje_clamp_to_s16(code * filter->scale));
    }
    return 0;
}

/* TODO: [near miss] 96.47%; unrolled word and halfword copy load/store staging differs. */
s32 adxsje_write68(const void* source, s32 element_size, s32 count, SJ* output)
{
    s32 index;
    s32 byte_count;
    const s32* source32;
    s32* destination32;
    const s16* source16;
    s16* destination16;
    SJCK chunk;

    byte_count = element_size * count;
    output->interface->get_chunk(output, 0, byte_count, &chunk);
    if (chunk.len < byte_count) {
        output->interface->unget_chunk(output, 0, &chunk);
        return 0;
    }
    if (element_size == 4) {
        destination32 = (s32*)chunk.data;
        source32 = source;
        for (index = 0; index < count; index++) {
            *destination32++ = *source32++;
        }
    } else if (element_size == 2) {
        destination16 = (s16*)chunk.data;
        source16 = source;
        for (index = 0; index < count; index++) {
            *destination16++ = *source16++;
        }
    } else if (element_size == 1) {
        memcpy(chunk.data, source, (u16)count);
    } else {
        for (;;) {
        }
    }
    output->interface->put_chunk(output, 1, &chunk);
    return count;
}
