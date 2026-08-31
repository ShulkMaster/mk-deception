#include "cri/adx_basic.h"
#include "cri/adx_dcd.h"
#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

enum {
    ADXSJD_MAX_HANDLES = 16,
    ADXSJD_MAX_CHANNELS = 2,
    ADXSJD_MAX_HEADER_SIZE = 0xC800,
    ADXSJD_MAX_OUTPUT_CHUNK = 0x4000
};

typedef struct AhxDecoder AhxDecoder;
typedef struct AdxBasicAhx AdxBasicAhx;

typedef struct AdxBasicDecoderExt {
    AdxBasicDecoder base;
    s16 default_key[3];
    s16 snapshot_key[3];
    s16 delay_left[2];
    s16 delay_right[2];
    AhxDecoder* ahx_decoder;
    s32 ahx_max_decoded_samples;
    s32 ahx_max_decoded_blocks;
    s32 ainf_length;
    u8 ainf[16];
    s16 default_out_volume;
    s16 default_pan[2];
    u8 reserved_DA[2];
    void* pl2_context;
    u8 reserved_E0[8];
    s32 last_notified_data_length;
    s32 field_EC;
    void (*notify)(void*, s32, s32);
    void* notify_object;
} AdxBasicDecoderExt;

typedef void (*AdxSjdTrapCallback)(void* object);
typedef void (*AdxSjdOutputCallback)(
    void* object, s32 channel, u8* data, s32 length);
typedef void (*AdxSjdSetFrequency)(AdxBasicDecoderExt* decoder, s32 frequency);

typedef struct AdxSjdHandle {
    s8 used;
    s8 status;
    s8 channel_count;
    s8 wait_for_input;
    AdxBasicDecoderExt* decoder;
    SJ* input;
    SJ* output[ADXSJD_MAX_CHANNELS];
    SJCK input_chunk;
    SJCK output_chunk[ADXSJD_MAX_CHANNELS];
    s32 decoded_samples;
    s32 decoded_data_length;
    s32 decode_position;
    s32 max_decode_samples;
    s32 trap_num_samples;
    s32 trap_count;
    s32 trap_data_length;
    AdxSjdTrapCallback trap_callback;
    void* trap_object;
    AdxSjdOutputCallback output_callback;
    void* output_object;
    u8 spsd_info[0x40];
    s32 header_length;
    s32 link_switch;
    s32 pending_leading_samples;
    s32 pending_trailing_samples;
} AdxSjdHandle;

typedef char AdxBasicDecoderExtSizeCheck[
    sizeof(AdxBasicDecoderExt) == 0xF8 ? 1 : -1];
typedef char AdxSjdHandleSizeCheck[
    sizeof(AdxSjdHandle) == 0xA8 ? 1 : -1];

extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void ADXERR_CallErrFunc2(const char* message1, const char* message2);

extern void ADXB_Init(void);
extern AdxBasicDecoderExt* ADXB_Create(
    s32 max_channels, s16* pcm_buffer, s32 pcm_size, s32 pcm_distance);
extern void ADXB_Destroy(AdxBasicDecoderExt* decoder);
extern void ADXB_ExecHndl(AdxBasicDecoderExt* decoder);
extern void ADXB_Reset(AdxBasicDecoderExt* decoder);
extern void ADXB_Stop(AdxBasicDecoderExt* decoder);
extern void ADXB_Start(AdxBasicDecoderExt* decoder);
extern void ADXB_EntryData(
    AdxBasicDecoderExt* decoder, signed char* input, s32 length);
extern s32 ADXB_GetStat(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetDecNumSmpl(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetDecDtLen(AdxBasicDecoderExt* decoder);
extern void ADXB_RestoreSnapshot(AdxBasicDecoderExt* decoder);
extern void ADXB_TakeSnapshot(AdxBasicDecoderExt* decoder);
extern s16 ADXB_GetDefPan(AdxBasicDecoderExt* decoder, s32 channel);
extern s16 ADXB_GetDefOutVol(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetAinfLen(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetLpEndOfst(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetLpEndPos(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetLpStartOfst(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetLpStartPos(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetNumLoop(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetTotalNumSmpl(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetBlkSmpl(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetOutBps(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetNumChan(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetSfreq(AdxBasicDecoderExt* decoder);
extern s32 ADXB_GetFormat(AdxBasicDecoderExt* decoder);
extern s16* ADXB_GetPcmBuf(AdxBasicDecoderExt* decoder);
extern void ADXB_EntryGetWrFunc(
    AdxBasicDecoderExt* decoder, AdxGetWriteInfo function, void* object);
extern s32 ADXB_DecodeHeader(
    AdxBasicDecoderExt* decoder, signed char* input, s32 length);
extern void ADXB_SetDefPrm(AdxBasicDecoderExt* decoder);

extern void ADXB_AhxTermSupply(AdxBasicAhx* decoder);
extern void ADXB_SetAhxDecSmpl(AdxBasicAhx* decoder, s32 samples);
extern void ADXB_SetAhxInSj(AdxBasicAhx* decoder, SJ* input);

extern u8* SJRBF_GetBufPtr(SJ* sj);
extern s32 SJRBF_GetBufSize(SJ* sj);
extern s32 SJRBF_GetXtrSize(SJ* sj);

static const char adxsjd_decode_header_error[] =
    "E03010901 ADXB_DecodeHeader: ";
static const char adxsjd_unsupported_format[] =
    "Can not decode this file format.";

AdxSjdHandle adxsjd_obj[ADXSJD_MAX_HANDLES];
AdxSjdSetFrequency pl2setsfreqfunc = 0;

void ADXSJD_ExecHndl(AdxSjdHandle* handle);
void adxsjd_decexec_start(AdxSjdHandle* handle);
void adxsjd_get_wr(
    void* object, int* write_position, int* writable_samples,
    int* limit_samples);
void adxsjd_decode_prep(AdxSjdHandle* handle);

void ADXSJD_RestoreSnapshot(AdxSjdHandle* handle)
{
    ADXB_RestoreSnapshot(handle->decoder);
}

void ADXSJD_TakeSnapshot(AdxSjdHandle* handle)
{
    ADXB_TakeSnapshot(handle->decoder);
}

u8* ADXSJD_GetSpsdInfo(AdxSjdHandle* handle)
{
    return handle->spsd_info;
}

s16 ADXSJD_GetDefPan(AdxSjdHandle* handle, s32 channel)
{
    if (ADXB_GetAinfLen(handle->decoder) > 0 &&
        (handle->status == 2 || handle->status == 3)) {
        return ADXB_GetDefPan(handle->decoder, channel);
    }
    return -128;
}

s16 ADXSJD_GetDefOutVol(AdxSjdHandle* handle)
{
    if (ADXB_GetAinfLen(handle->decoder) > 0 &&
        (handle->status == 2 || handle->status == 3)) {
        return ADXB_GetDefOutVol(handle->decoder);
    }
    return 0;
}

s32 ADXSJD_GetLpEndOfst(AdxSjdHandle* handle)
{
    return ADXB_GetLpEndOfst(handle->decoder);
}

s32 ADXSJD_GetLpEndPos(AdxSjdHandle* handle)
{
    return ADXB_GetLpEndPos(handle->decoder);
}

s32 ADXSJD_GetLpStartOfst(AdxSjdHandle* handle)
{
    if (handle == 0) {
        return 0;
    }
    return ADXB_GetLpStartOfst(handle->decoder);
}

s32 ADXSJD_GetLpStartPos(AdxSjdHandle* handle)
{
    return ADXB_GetLpStartPos(handle->decoder);
}

s32 ADXSJD_GetNumLoop(AdxSjdHandle* handle)
{
    return ADXB_GetNumLoop(handle->decoder);
}

s32 ADXSJD_GetTotalNumSmpl(AdxSjdHandle* handle)
{
    return ADXB_GetTotalNumSmpl(handle->decoder);
}

s32 ADXSJD_GetBlkSmpl(AdxSjdHandle* handle)
{
    return ADXB_GetBlkSmpl(handle->decoder);
}

s32 ADXSJD_GetOutBps(AdxSjdHandle* handle)
{
    return ADXB_GetOutBps(handle->decoder);
}

s32 ADXSJD_GetNumChan(AdxSjdHandle* handle)
{
    return ADXB_GetNumChan(handle->decoder);
}

s32 ADXSJD_GetSfreq(AdxSjdHandle* handle)
{
    return ADXB_GetSfreq(handle->decoder);
}

s32 ADXSJD_GetFormat(AdxSjdHandle* handle)
{
    return ADXB_GetFormat(handle->decoder);
}

void ADXSJD_SetTrapDtLen(AdxSjdHandle* handle, s32 length)
{
    handle->trap_data_length = length;
}

void ADXSJD_SetTrapCnt(AdxSjdHandle* handle, s32 count)
{
    handle->trap_count = count;
}

void ADXSJD_SetTrapNumSmpl(AdxSjdHandle* handle, s32 samples)
{
    handle->trap_num_samples = samples;
}

void ADXSJD_EntryTrapFunc(
    AdxSjdHandle* handle, AdxSjdTrapCallback callback, void* object)
{
    handle->trap_callback = callback;
    handle->trap_object = object;
}

void ADXSJD_SetLnkSw(AdxSjdHandle* handle, s32 link_switch)
{
    handle->link_switch = link_switch;
}

void ADXSJD_SetDecPos(AdxSjdHandle* handle, s32 position)
{
    handle->decode_position = position;
}

s32 ADXSJD_GetDecNumSmpl(AdxSjdHandle* handle)
{
    return handle->decoded_samples;
}

void ADXSJD_ExecServer(void)
{
    s32 index;

    for (index = 0; index < ADXSJD_MAX_HANDLES; index++) {
        if (adxsjd_obj[index].used == 1) {
            ADXSJD_ExecHndl(&adxsjd_obj[index]);
        }
    }
}

void ADXSJD_ExecHndl(AdxSjdHandle* handle)
{
    AdxBasicDecoderExt* decoder;
    AdxBasicDecoderExt* completed_decoder;
    SJ* input;
    SJCK leading_chunk;
    SJCK decoded_chunk;
    SJCK remainder;
    SJCK trailing_chunk;
    s32 channel;
    s32 bytes;
    s32 samples;
    s32 decoded_samples;
    s32 decoded_length;
    s32 output_samples;
    s32 format;

    if (handle->pending_leading_samples > 0) {
        ADXCRS_Lock();
        bytes = handle->pending_leading_samples * 2;
        for (channel = 0; channel < handle->channel_count; channel++) {
            handle->output[channel]->interface->get_chunk(
                handle->output[channel], 0, 0x7FFFFFFF, &leading_chunk);
            bytes = bytes < leading_chunk.len ? bytes : leading_chunk.len;
            handle->output[channel]->interface->unget_chunk(
                handle->output[channel], 0, &leading_chunk);
        }
        samples = bytes / 2;
        bytes = samples * 2;
        if (bytes > 0) {
            for (channel = 0; channel < handle->channel_count; channel++) {
                handle->output[channel]->interface->get_chunk(
                    handle->output[channel], 0, bytes, &leading_chunk);
                memset(leading_chunk.data, 0, bytes);
                handle->output[channel]->interface->put_chunk(
                    handle->output[channel], 1, &leading_chunk);
            }
            handle->pending_leading_samples -= samples;
        }
        ADXCRS_Unlock();
    }

    if (handle->status == 2) {
        decoder = handle->decoder;
        if (ADXB_GetStat(decoder) == 0) {
            adxsjd_decexec_start(handle);
        }
        ADXB_ExecHndl(decoder);
        if (ADXB_GetStat(decoder) == 3) {
            completed_decoder = handle->decoder;
            input = handle->input;
            output_samples = ADXB_GetTotalNumSmpl(completed_decoder);
            decoded_length = ADXB_GetDecDtLen(completed_decoder);
            decoded_samples = ADXB_GetDecNumSmpl(completed_decoder);
            output_samples -= handle->decode_position;
            if (decoded_samples < output_samples) {
                output_samples = decoded_samples;
            }

            SJ_SplitChunk(
                &handle->input_chunk, decoded_length, &decoded_chunk,
                &remainder);
            input->interface->put_chunk(input, 0, &decoded_chunk);
            input->interface->unget_chunk(input, 1, &remainder);

            for (channel = 0; channel < ADXB_GetNumChan(completed_decoder);
                 channel++) {
                SJ_SplitChunk(
                    &handle->output_chunk[channel], output_samples * 2,
                    &decoded_chunk, &remainder);
                if (handle->output_callback != 0) {
                    handle->output_callback(
                        handle->output_object, channel, decoded_chunk.data,
                        decoded_chunk.len);
                }
                handle->output[channel]->interface->put_chunk(
                    handle->output[channel], 1, &decoded_chunk);
                handle->output[channel]->interface->unget_chunk(
                    handle->output[channel], 0, &remainder);
            }

            handle->decoded_samples += output_samples;
            handle->decoded_data_length += decoded_length;
            handle->decode_position += output_samples;
            handle->trap_count += output_samples;
            handle->trap_data_length += decoded_length;
            ADXB_Reset(completed_decoder);
        }

        format = decoder->base.format_type;
        if (format == 10 || format == 20 ||
            (u16)(format - 11) <= 1 || format == 15) {
            completed_decoder = handle->decoder;
            output_samples = ADXB_GetTotalNumSmpl(completed_decoder);
            decoded_length = ADXB_GetDecDtLen(completed_decoder);
            decoded_samples = ADXB_GetDecNumSmpl(completed_decoder);
            output_samples -= handle->decode_position;
            if (decoded_samples < output_samples) {
                output_samples = decoded_samples;
            }
            handle->decoded_samples += output_samples;
            handle->decoded_data_length += decoded_length;
            handle->decode_position += output_samples;
        }
    } else if (handle->status == 1) {
        adxsjd_decode_prep(handle);
    }

    if (handle->pending_trailing_samples > 0) {
        ADXCRS_Lock();
        bytes = handle->pending_trailing_samples * 2;
        for (channel = 0; channel < handle->channel_count; channel++) {
            handle->output[channel]->interface->get_chunk(
                handle->output[channel], 1, 0x7FFFFFFF, &trailing_chunk);
            bytes = bytes < trailing_chunk.len ? bytes : trailing_chunk.len;
            handle->output[channel]->interface->unget_chunk(
                handle->output[channel], 1, &trailing_chunk);
        }
        samples = bytes / 2;
        bytes = samples * 2;
        if (bytes > 0) {
            for (channel = 0; channel < handle->channel_count; channel++) {
                handle->output[channel]->interface->get_chunk(
                    handle->output[channel], 1, bytes, &trailing_chunk);
                handle->output[channel]->interface->put_chunk(
                    handle->output[channel], 0, &trailing_chunk);
            }
            handle->pending_trailing_samples -= samples;
        }
        ADXCRS_Unlock();
    }
}

void adxsjd_decexec_start(AdxSjdHandle* handle)
{
    AdxBasicDecoderExt* decoder = handle->decoder;
    SJ* input = handle->input;
    SJCK remainder;
    s16 footer_length;
    s32 length;
    s32 zero_length;

    if (handle->trap_num_samples >= 0 &&
        handle->trap_count >= handle->trap_num_samples &&
        handle->trap_callback != 0) {
        handle->trap_callback(handle->trap_object);
    }

    if (handle->wait_for_input == 1 &&
        input->interface->get_num_data(input, 1) == 0) {
        handle->status = 3;
        return;
    }

    input->interface->get_chunk(
        input, 1, 0x7FFFFFFF, &handle->input_chunk);
    if (ADXB_GetFormat(decoder) == 0 &&
        handle->input_chunk.len >= 4 &&
        *(u16*)handle->input_chunk.data == 0x8001) {
        handle->status = 3;
        if (ADX_DecodeFooter(
                (signed char*)handle->input_chunk.data,
                handle->input_chunk.len, &footer_length) == 0) {
            if (footer_length > handle->input_chunk.len) {
                input->interface->unget_chunk(
                    input, 1, &handle->input_chunk);
                return;
            }
            SJ_SplitChunk(
                &handle->input_chunk, footer_length, &handle->input_chunk,
                &remainder);
            input->interface->put_chunk(input, 0, &handle->input_chunk);
            input->interface->unget_chunk(input, 1, &remainder);
        }

        if (handle->link_switch != 0) {
            for (;;) {
                input->interface->get_chunk(
                    input, 1, 0x7FFFFFFF, &handle->input_chunk);
                length = handle->input_chunk.len;
                if (length == 0) {
                    return;
                }
                zero_length = 0;
                while (zero_length < length &&
                       ((signed char*)handle->input_chunk.data)[zero_length] ==
                           0) {
                    zero_length++;
                }
                SJ_SplitChunk(
                    &handle->input_chunk, zero_length, &handle->input_chunk,
                    &remainder);
                input->interface->put_chunk(input, 0, &handle->input_chunk);
                input->interface->unget_chunk(input, 1, &remainder);
                if (zero_length < length) {
                    return;
                }
            }
        }
        return;
    }

    if (handle->decode_position >= ADXB_GetTotalNumSmpl(decoder)) {
        handle->status = 3;
        input->interface->unget_chunk(input, 1, &handle->input_chunk);
        return;
    }

    if (handle->output[0]->interface->get_num_data(handle->output[0], 0) / 2 <
        ADXB_GetBlkSmpl(decoder)) {
        input->interface->unget_chunk(input, 1, &handle->input_chunk);
        return;
    }

    if (ADXB_GetFormat(decoder) == 10) {
        input->interface->unget_chunk(input, 1, &handle->input_chunk);
    }
    ADXB_EntryData(
        decoder, (signed char*)handle->input_chunk.data,
        handle->input_chunk.len);
    ADXB_Start(decoder);
}

void adxsjd_get_wr(
    void* object, int* write_position, int* writable_samples,
    int* limit_samples)
{
    AdxSjdHandle* handle = object;
    SJ* first_output = handle->output[0];
    s32 channel;
    s32 samples;
    s32 available_samples;

    for (channel = 0; channel < ADXB_GetNumChan(handle->decoder); channel++) {
        handle->output[channel]->interface->get_chunk(
            handle->output[channel], 0, ADXSJD_MAX_OUTPUT_CHUNK,
            &handle->output_chunk[channel]);
    }

    *write_position =
        (handle->output_chunk[0].data - SJRBF_GetBufPtr(first_output)) / 2;
    samples = handle->max_decode_samples;
    available_samples = handle->output_chunk[0].len / 2;
    if (available_samples < samples) {
        samples = available_samples;
    }
    *writable_samples = samples;
    if (handle->trap_num_samples >= 0) {
        *limit_samples = handle->trap_num_samples - handle->trap_count;
    } else {
        *limit_samples = 0x1FFFFFFF;
    }
    (void)ADXB_GetPcmBuf(handle->decoder);
}

void adxsjd_decode_prep(AdxSjdHandle* handle)
{
    AdxBasicDecoderExt* decoder = handle->decoder;
    SJ* input = handle->input;
    SJCK chunk;
    SJCK other;
    s32 skip;
    s32 remaining;
    s32 header_length;
    s32 copy_length;

    input->interface->get_chunk(input, 1, ADXSJD_MAX_HEADER_SIZE, &chunk);
    skip = 0;
    remaining = chunk.len;
    while (remaining > 0) {
        if (((signed char*)chunk.data)[skip] != 0) {
            break;
        }
        skip++;
        remaining--;
    }
    SJ_SplitChunk(&chunk, skip, &other, &chunk);
    input->interface->put_chunk(input, 0, &other);

    if (chunk.len < 16) {
        input->interface->unget_chunk(input, 1, &chunk);
        return;
    }

    header_length = ADXB_DecodeHeader(
        decoder, (signed char*)chunk.data, chunk.len);
    if (header_length == 0 || header_length > chunk.len) {
        input->interface->unget_chunk(input, 1, &chunk);
        return;
    }

    if (header_length < 0) {
        if (decoder->base.field_9A != 0) {
            ADXB_SetDefPrm(decoder);
            header_length = 0;
        } else {
            input->interface->unget_chunk(input, 1, &chunk);
            ADXERR_CallErrFunc2(
                adxsjd_decode_header_error, adxsjd_unsupported_format);
            handle->status = 4;
            return;
        }
    }

    handle->header_length = header_length;
    if (ADXB_GetFormat(decoder) == 4) {
        handle->wait_for_input = 1;
    }
    if (ADXB_GetFormat(decoder) == 2) {
        copy_length = chunk.len < (s32)sizeof(handle->spsd_info)
                          ? chunk.len
                          : sizeof(handle->spsd_info);
        memcpy(handle->spsd_info, chunk.data, copy_length);
    }

    {
        s32 format = ADXB_GetFormat(decoder);
        if ((u32)(format - 10) <= 2 || format == 20 || format == 15) {
            input->interface->unget_chunk(input, 1, &chunk);
        } else {
            SJ_SplitChunk(&chunk, header_length, &chunk, &other);
            input->interface->put_chunk(input, 0, &chunk);
            input->interface->unget_chunk(input, 1, &other);
        }
    }

    if (decoder->pl2_context != 0 && pl2setsfreqfunc != 0) {
        pl2setsfreqfunc(decoder, decoder->base.sample_rate);
    }
    handle->status = 2;
}

void ADXSJD_Stop(AdxSjdHandle* handle)
{
    ADXB_Stop(handle->decoder);
    handle->status = 0;
}

void ADXSJD_Start(AdxSjdHandle* handle)
{
    handle->header_length = 0;
    handle->decoded_samples = 0;
    handle->decoded_data_length = 0;
    handle->decode_position = 0;
    handle->max_decode_samples = 0x7FFFFFFF;
    handle->trap_num_samples = -1;
    handle->trap_count = 0;
    handle->trap_data_length = 0;
    handle->wait_for_input = 0;
    handle->pending_leading_samples = 0;
    handle->pending_trailing_samples = 0;
    handle->status = 1;
}

void ADXSJD_TermSupply(AdxSjdHandle* handle)
{
    ADXB_AhxTermSupply((AdxBasicAhx*)handle->decoder);
}

void ADXSJD_SetMaxDecSmpl(AdxSjdHandle* handle, s32 samples)
{
    handle->max_decode_samples = samples;
    ADXB_SetAhxDecSmpl((AdxBasicAhx*)handle->decoder, samples);
}

void ADXSJD_SetInSj(AdxSjdHandle* handle, SJ* input)
{
    handle->input = input;
    ADXB_SetAhxInSj((AdxBasicAhx*)handle->decoder, input);
}

s32 ADXSJD_GetStat(AdxSjdHandle* handle)
{
    return handle->status;
}

void ADXSJD_Destroy(AdxSjdHandle* handle)
{
    AdxBasicDecoderExt* decoder;

    if (handle != 0) {
        decoder = handle->decoder;
        if (decoder != 0) {
            handle->decoder = 0;
            ADXB_Destroy(decoder);
        }
        ADXCRS_Lock();
        memset(handle, 0, sizeof(*handle));
        ADXCRS_Unlock();
    }
}

AdxSjdHandle* ADXSJD_Create(SJ* input, s32 channel_count, SJ** output)
{
    AdxSjdHandle* handle;
    SJ* first_output;
    u8* pcm_buffer;
    s32 pcm_size;
    s32 pcm_extra;
    s32 index;

    first_output = output[0];
    for (index = 0; index < ADXSJD_MAX_HANDLES; index++) {
        if (adxsjd_obj[index].used == 0) {
            break;
        }
    }
    if (index == ADXSJD_MAX_HANDLES) {
        return 0;
    }
    handle = &adxsjd_obj[index];

    pcm_buffer = SJRBF_GetBufPtr(first_output);
    pcm_size = SJRBF_GetBufSize(first_output) / 2;
    pcm_extra = SJRBF_GetXtrSize(first_output) / 2;
    handle->decoder = ADXB_Create(
        channel_count, (s16*)pcm_buffer, pcm_size, pcm_size + pcm_extra);
    if (handle->decoder == 0) {
        return 0;
    }

    ADXB_EntryGetWrFunc(handle->decoder, adxsjd_get_wr, handle);
    handle->input = input;
    handle->channel_count = channel_count;
    for (index = 0; index < channel_count; index++) {
        handle->output[index] = output[index];
    }
    handle->status = 0;
    handle->header_length = 0;
    handle->decoded_samples = 0;
    handle->decoded_data_length = 0;
    handle->decode_position = 0;
    handle->max_decode_samples = 0x7FFFFFFF;
    handle->trap_num_samples = -1;
    handle->trap_count = 0;
    handle->trap_data_length = 0;
    handle->wait_for_input = 0;
    handle->pending_leading_samples = 0;
    handle->pending_trailing_samples = 0;
    handle->trap_callback = 0;
    handle->trap_object = 0;
    handle->output_callback = 0;
    handle->output_object = 0;
    handle->used = 1;
    return handle;
}

void ADXSJD_Finish(void)
{
    memset(adxsjd_obj, 0, sizeof(adxsjd_obj));
}

void ADXSJD_Init(void)
{
    ADXB_Init();
    memset(adxsjd_obj, 0, sizeof(adxsjd_obj));
}
