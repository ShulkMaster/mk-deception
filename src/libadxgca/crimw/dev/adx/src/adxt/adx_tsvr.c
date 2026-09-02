#include "cri/adx_dcd.h"
#include "cri/adxt_internal.h"
#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

enum {
    ADXT_STATUS_DECODING_HEADER = 1,
    ADXT_STATUS_BUFFERING = 2,
    ADXT_STATUS_PLAYING = 3,
    ADXT_STATUS_DRAINING = 4,
    ADXT_STATUS_ERROR = 6,
    ADXT_STREAM_TYPE_FILE = 0,
    ADXT_STREAM_TYPE_RANGE = 1,
    ADXT_STREAM_TYPE_MEMORY = 2,
    ADXT_STREAM_TYPE_LINKED = 3,
    ADXSJD_STATUS_HEADER_READY = 2,
    ADXSJD_STATUS_INPUT_END = 3,
    ADXSTM_STATUS_READING = 2,
    ADXSTM_STATUS_END = 3,
    ADXSTM_STATUS_ERROR = 4,
    ADXT_SECTOR_SIZE = 0x800,
    ADXT_MIN_PLAY_DATA = 0x40
};

typedef void (*AdxtEndDecodeInfoCallback)(
    ADXTHandle* handle, s32 sample_rate, s32 channels, s32 total_samples);

extern s32 adxt_vsync_cnt;

extern void ADXERR_CallErrFunc1(const char* message);
extern void ADXERR_CallErrFunc2(const char* message1, const char* message2);
extern void ADXERR_ItoA2(
    s32 value1, s32 value2, signed char* string, s32 length);

extern s32 ADXSJD_GetStat(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetNumChan(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetBlkSmpl(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetSfreq(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetNumLoop(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetLpEndOfst(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetLpEndPos(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetLpStartOfst(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetLpStartPos(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetTotalNumSmpl(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetOutBps(AdxSjdHandle* decoder);
extern s16 ADXSJD_GetDefOutVol(AdxSjdHandle* decoder);
extern s16 ADXSJD_GetDefPan(AdxSjdHandle* decoder, s32 channel);
extern s32 ADXSJD_GetFormat(AdxSjdHandle* decoder);
extern u8* ADXSJD_GetSpsdInfo(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetDecNumSmpl(AdxSjdHandle* decoder);
extern void ADXSJD_SetMaxDecSmpl(AdxSjdHandle* decoder, s32 samples);
extern void ADXSJD_SetTrapNumSmpl(AdxSjdHandle* decoder, s32 samples);
extern void ADXSJD_SetTrapDtLen(AdxSjdHandle* decoder, s32 length);
extern void ADXSJD_SetTrapCnt(AdxSjdHandle* decoder, s32 count);
extern void ADXSJD_SetDecPos(AdxSjdHandle* decoder, s32 position);
extern void ADXSJD_EntryTrapFunc(
    AdxSjdHandle* decoder, void (*callback)(void* object), void* object);
extern void ADXSJD_TermSupply(AdxSjdHandle* decoder);
extern void ADXSJD_Stop(AdxSjdHandle* decoder);
extern void ADXSJD_Start(AdxSjdHandle* decoder);
extern void ADXSJD_ExecHndl(AdxSjdHandle* decoder);
extern void ADXSJD_RestoreSnapshot(AdxSjdHandle* decoder);
extern void ADXSJD_TakeSnapshot(AdxSjdHandle* decoder);

extern s32 ADXSTM_GetStat(ADXStream* stream);
extern void ADXSTM_SetEos(ADXStream* stream, s32 sector);
extern void ADXSTM_EntryEosFunc(
    ADXStream* stream, void (*callback)(void* object), void* object);
extern s32 ADXSTM_Seek(ADXStream* stream, s32 sector);

extern s32 ADXRNA_GetNumData(AXRNAHandle* rna);
extern s32 ADXRNA_GetNumRoom(AXRNAHandle* rna);
extern void ADXRNA_SetPlaySw(AXRNAHandle* rna, s32 enabled);
extern void ADXRNA_SetTransSw(AXRNAHandle* rna, s32 enabled);
extern void ADXRNA_SetBitPerSmpl(AXRNAHandle* rna, s32 bits);
extern void ADXRNA_SetSfreq(AXRNAHandle* rna, s32 sample_rate);
extern void ADXRNA_SetNumChan(AXRNAHandle* rna, s32 channels);
extern void ADXRNA_SetTotalNumSmpl(AXRNAHandle* rna, s32 samples);
extern void ADXRNA_SetOutVol(AXRNAHandle* rna, s32 volume);
extern void ADXRNA_SetOutPan(AXRNAHandle* rna, s32 channel, s32 pan);
extern s32 ADXRNA_SetStmHdInfo(AXRNAHandle* rna, void* stream_header);

extern s32 ADXT_GetNumChan(ADXTHandle* handle);
extern s32 ADXT_GetStat(ADXTHandle* handle);
extern void ADXT_GetTranspose(
    ADXTHandle* handle, s32* transpose, s32* fine_transpose);
extern void ADXT_SetTranspose(
    ADXTHandle* handle, s32 transpose, s32 fine_transpose);
extern void ADXT_SetLnkSw(ADXTHandle* handle, s8 enabled);
extern void ADXT_Stop(ADXTHandle* handle);
extern void adxt_start_stm(
    ADXTHandle* handle, const char* filename, void* directory,
    s32 file_offset, s32 file_sectors);

extern void ADXAMP_SetSfreq(ADX_AMP* amplifier, s32 sample_rate);
extern s32 LSC_GetStat(LSCObject* controller);

void adxt_stat_decinfo(ADXTHandle* handle);
void adxt_nlp_trap_entry(void* object);
void adxt_set_outpan(ADXTHandle* handle);
void adxt_eos_entry(void* object);
void adxt_trap_entry(void* object);
void adxt_trap_entry_lps(void* object);

static const char adxt_exec_parameter_error[] =
    "E02080842 ADXT_ExecHndl: parameter error";
static const char adxt_channel_count_error[] =
    "E9081001 adxt_stat_decinfo: can't play this number of channels";
static const char adxt_loop_data_error[] =
    "E8101201 adxt_trap_entry: not enough data";

static AdxtEndDecodeInfoCallback adxt_enddecinfo_cbfn = 0;
s32 adxt_dbg_rna_ndata = 0;
s32 adxt_dbg_ndt = 0;
s32 adxt_dbg_nch = 0;

void ADXT_ExecHndl(ADXTHandle* handle)
{
    SJCK chunk;
    s32 decoder_channels;
    s32 channel;
    s32 buffer_data;
    s32 buffer_room;
    s32 bytes;

    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_exec_parameter_error);
        return;
    }

    if (handle->status == ADXT_STATUS_PLAYING) {
        if (ADXSJD_GetStat(handle->decoder) == ADXSJD_STATUS_INPUT_END) {
            decoder_channels = ADXSJD_GetNumChan(handle->decoder);
            adxt_dbg_nch = decoder_channels;
            for (channel = 0; channel < decoder_channels; channel++) {
                adxt_dbg_ndt = handle->output_sj[channel]->interface->get_num_data(
                    handle->output_sj[channel], 1);
                if (adxt_dbg_ndt >= ADXT_MIN_PLAY_DATA) {
                    break;
                }
            }
            if (channel == decoder_channels) {
                ADXRNA_SetTransSw(handle->rna, 0);
                handle->status = ADXT_STATUS_DRAINING;
            }
        }
    } else if (handle->status == ADXT_STATUS_DECODING_HEADER) {
        adxt_stat_decinfo(handle);
    } else if (handle->status == ADXT_STATUS_BUFFERING) {
        buffer_data = ADXRNA_GetNumData(handle->rna);
        buffer_room = ADXRNA_GetNumRoom(handle->rna);
        if (buffer_data >= handle->maximum_decode_samples * 2 ||
            buffer_room <= ADXSJD_GetBlkSmpl(handle->decoder) ||
            ADXSJD_GetStat(handle->decoder) == ADXSJD_STATUS_INPUT_END) {
            if (handle->suppress_playback == 0) {
                if (handle->paused == 0) {
                    ADXRNA_SetPlaySw(handle->rna, 1);
                    handle->playback_time = 0;
                    handle->playback_start_vsync = adxt_vsync_cnt;
                }
                handle->status = ADXT_STATUS_PLAYING;
            }
            handle->decoder_ready = 1;
        }
        if (ADXSJD_GetStat(handle->decoder) == ADXSJD_STATUS_INPUT_END) {
            decoder_channels = ADXT_GetNumChan(handle);
            bytes = handle->maximum_decode_samples * decoder_channels * 2;
            for (channel = 0; channel < decoder_channels; channel++) {
                handle->output_sj[channel]->interface->get_chunk(
                    handle->output_sj[channel], 0, bytes, &chunk);
                memset(chunk.data, 0, chunk.len);
                handle->output_sj[channel]->interface->put_chunk(
                    handle->output_sj[channel], 1, &chunk);
            }
        }
    } else if (handle->status == ADXT_STATUS_DRAINING) {
        adxt_dbg_rna_ndata = ADXRNA_GetNumData(handle->rna);
        if (ADXRNA_GetNumData(handle->rna) <= 0) {
            ADXRNA_SetPlaySw(handle->rna, 0);
            handle->status = 5;
        }
    }

    if (handle->stream != 0 && ADXT_GetStat(handle) != 0) {
        if (handle->stream_type == ADXT_STREAM_TYPE_FILE ||
            handle->stream_type == ADXT_STREAM_TYPE_RANGE) {
            if (ADXSTM_GetStat(handle->stream) == ADXSTM_STATUS_END) {
                ADXSJD_TermSupply(handle->decoder);
            }
        } else if (handle->stream_type == ADXT_STREAM_TYPE_MEMORY) {
            ADXSJD_TermSupply(handle->decoder);
        }
    }
    if (handle->stream != 0 &&
        ADXSTM_GetStat(handle->stream) == ADXSTM_STATUS_ERROR) {
        handle->error_code = -1;
        handle->status = ADXT_STATUS_ERROR;
    }
    if (handle->linked_stream_controller != 0 &&
        LSC_GetStat(handle->linked_stream_controller) == ADXSTM_STATUS_END) {
        handle->error_code = -1;
        handle->status = ADXT_STATUS_ERROR;
    }
}

void adxt_stat_decinfo(ADXTHandle* handle)
{
    signed char channel_error[32];
    s32 transpose;
    s32 fine_transpose;
    AdxSjdHandle* decoder;
    s32 sample_rate;
    s32 loop_count;
    s32 block_samples;
    s32 loop_end_offset;
    s32 eos_sector;
    s32 channel_count;
    s32 total_samples;

    decoder = handle->decoder;
    transpose = 0;
    fine_transpose = 0;
    if ((handle->stream_type == ADXT_STREAM_TYPE_FILE ||
         handle->stream_type == ADXT_STREAM_TYPE_RANGE) &&
        handle->pending_stream_start == 1) {
        if (ADXSTM_GetStat(handle->stream) == ADXSTM_STATUS_READING) {
            return;
        }
        if (handle->stream_sj != 0) {
            handle->stream_sj->interface->reset(handle->stream_sj);
        }
        adxt_start_stm(
            handle, handle->pending_filename, handle->pending_directory,
            handle->pending_file_offset, handle->pending_file_sectors);
        handle->pending_stream_start = 0;
    }
    if (ADXSJD_GetStat(decoder) != ADXSJD_STATUS_HEADER_READY) {
        return;
    }
    channel_count = ADXSJD_GetNumChan(decoder);
    if (channel_count > handle->maximum_channels) {
        ADXERR_ItoA2(
            channel_count, handle->maximum_channels, channel_error, 16);
        ADXERR_CallErrFunc2(adxt_channel_count_error, (char*)channel_error);
        ADXT_Stop(handle);
        return;
    }

    sample_rate = ADXSJD_GetSfreq(decoder);
    loop_count = ADXSJD_GetNumLoop(decoder);
    if (loop_count > 0) {
        handle->maximum_decode_samples =
            (sample_rate / handle->server_frequency) * 3;
    } else {
        handle->maximum_decode_samples =
            ((sample_rate / handle->server_frequency) * 3) / 2;
    }
    block_samples = ADXSJD_GetBlkSmpl(decoder) * 2;
    handle->maximum_decode_samples =
        block_samples *
        ((handle->maximum_decode_samples + block_samples) / block_samples);
    ADXSJD_SetMaxDecSmpl(decoder, handle->maximum_decode_samples);

    if (loop_count > 0) {
        if (handle->stream_type == ADXT_STREAM_TYPE_MEMORY) {
            handle->link_data_length = 0;
        } else {
            loop_end_offset = ADXSJD_GetLpEndOfst(decoder);
            handle->link_data_length =
                ADXT_SECTOR_SIZE - loop_end_offset % ADXT_SECTOR_SIZE;
            eos_sector =
                (loop_end_offset + ADXT_SECTOR_SIZE - 1) / ADXT_SECTOR_SIZE;
            handle->link_data_length %= ADXT_SECTOR_SIZE;
            handle->eos_sector = eos_sector;
            ADXSTM_SetEos(handle->stream, eos_sector);
            ADXSTM_EntryEosFunc(handle->stream, adxt_eos_entry, handle);
        }
        ADXSJD_GetLpEndPos(decoder);
        handle->loop_sample_count = ADXSJD_GetLpStartPos(decoder);
        ADXSJD_SetTrapNumSmpl(decoder, handle->loop_sample_count);
        ADXSJD_SetTrapDtLen(decoder, 0);
        ADXSJD_SetTrapCnt(decoder, 0);
        ADXSJD_EntryTrapFunc(decoder, adxt_trap_entry_lps, handle);
    } else {
        if (handle->stream != 0) {
            ADXSTM_SetEos(handle->stream, 0x7FFFFFFF);
        }
        ADXSJD_SetTrapNumSmpl(decoder, ADXSJD_GetTotalNumSmpl(decoder));
        ADXSJD_SetTrapDtLen(decoder, 0);
        ADXSJD_SetTrapCnt(decoder, 0);
        ADXSJD_EntryTrapFunc(decoder, adxt_nlp_trap_entry, handle);
    }

    sample_rate = ADXSJD_GetSfreq(decoder);
    channel_count = ADXSJD_GetNumChan(decoder);
    total_samples = ADXSJD_GetTotalNumSmpl(decoder);
    ADXRNA_SetBitPerSmpl(handle->rna, ADXSJD_GetOutBps(decoder));
    ADXRNA_SetSfreq(handle->rna, sample_rate);
    ADXRNA_SetNumChan(handle->rna, channel_count);
    ADXRNA_SetTotalNumSmpl(handle->rna, total_samples);
    ADXRNA_SetOutVol(
        handle->rna,
        handle->output_volume + ADXSJD_GetDefOutVol(handle->decoder));
    ADXT_GetTranspose(handle, &transpose, &fine_transpose);
    if (transpose != 0 || fine_transpose != 0) {
        ADXT_SetTranspose(handle, transpose, fine_transpose);
    }
    adxt_set_outpan(handle);
    if (handle->amplifier != 0) {
        ADXAMP_SetSfreq(handle->amplifier, sample_rate);
    }
    if (ADXSJD_GetFormat(decoder) == 2) {
        ADXRNA_SetStmHdInfo(handle->rna, ADXSJD_GetSpsdInfo(decoder));
    }
    ADXRNA_SetTransSw(handle->rna, 1);
    if (adxt_enddecinfo_cbfn != 0) {
        adxt_enddecinfo_cbfn(
            handle, sample_rate, channel_count, total_samples);
    }
    handle->status = ADXT_STATUS_BUFFERING;
}

void adxt_nlp_trap_entry(void* object)
{
    ADXTHandle* handle = (ADXTHandle*)object;
    AdxSjdHandle* decoder = handle->decoder;
    SJ* input = handle->input_sj;
    SJCK first_chunk;
    SJCK first_remainder;
    SJCK second_chunk;
    SJCK second_remainder;
    s16 second_info_length;
    s16 first_info_length;
    s32 first_info_status;
    s32 second_info_status;
    s32 first_consumed;

    if (handle->link_enabled == 0) {
        return;
    }
    second_info_length = 0;
    input->interface->get_chunk(
        input, 1, 0x7FFFFFFF, &first_chunk);
    input->interface->get_chunk(
        input, 1, 0x7FFFFFFF, &second_chunk);
    if (ADX_DecodeFooter(
            (signed char*)first_chunk.data, first_chunk.len,
            &first_info_length) != 0) {
        ADXT_SetLnkSw(handle, 0);
        input->interface->unget_chunk(input, 1, &second_chunk);
        input->interface->unget_chunk(input, 1, &first_chunk);
        return;
    }

    first_consumed = first_info_length;
    first_info_status = ADX_ScanInfoCode(
        (signed char*)first_chunk.data + first_info_length,
        first_chunk.len - first_info_length, &first_info_length);
    if (first_info_status == 0) {
        second_info_status = -1;
    } else {
        second_info_status = ADX_ScanInfoCode(
            (signed char*)second_chunk.data, second_chunk.len,
            &second_info_length);
    }
    first_consumed += first_info_length;
    if (first_info_status != 0 && second_info_status != 0) {
        input->interface->unget_chunk(input, 1, &second_chunk);
        input->interface->unget_chunk(input, 1, &first_chunk);
        ADXT_SetLnkSw(handle, 0);
        return;
    }
    if (first_info_status == 0) {
        input->interface->unget_chunk(input, 1, &second_chunk);
        SJ_SplitChunk(
            &first_chunk, first_consumed, &first_chunk, &first_remainder);
        input->interface->put_chunk(input, 0, &first_chunk);
        input->interface->unget_chunk(input, 1, &first_remainder);
    } else {
        input->interface->put_chunk(input, 0, &first_chunk);
        SJ_SplitChunk(
            &second_chunk, second_info_length,
            &second_chunk, &second_remainder);
        input->interface->put_chunk(input, 0, &second_chunk);
        input->interface->unget_chunk(input, 1, &second_remainder);
    }

    handle->linked_decoded_samples += ADXSJD_GetDecNumSmpl(decoder);
    ADXSJD_Stop(decoder);
    ADXSJD_Start(decoder);
    ADXSJD_ExecHndl(decoder);
    if (ADXSJD_GetStat(decoder) != ADXSJD_STATUS_HEADER_READY) {
        ADXT_SetLnkSw(handle, 0);
        return;
    }
    ADXSJD_SetMaxDecSmpl(decoder, handle->maximum_decode_samples);
    ADXSJD_SetTrapNumSmpl(decoder, ADXSJD_GetTotalNumSmpl(decoder));
    ADXSJD_SetTrapDtLen(decoder, 0);
    ADXSJD_SetTrapCnt(decoder, 0);
}

void adxt_set_outpan(ADXTHandle* handle)
{
    s32 default_pan[2];
    s32 channel_count;
    s32 channel;
    s32 pan;

    channel_count = ADXSJD_GetNumChan(handle->decoder);
    for (channel = 0; channel < 2; channel++) {
        default_pan[channel] = ADXSJD_GetDefPan(handle->decoder, channel);
    }
    if (channel_count == 1) {
        pan = handle->output_pan[0];
        if (pan == -128 && default_pan[0] == -128) {
            ADXRNA_SetOutPan(handle->rna, 0, 0);
        } else if (pan != -128 && default_pan[0] == -128) {
            ADXRNA_SetOutPan(handle->rna, 0, pan);
        } else if (pan == -128 && default_pan[0] != -128) {
            ADXRNA_SetOutPan(handle->rna, 0, default_pan[0]);
        } else {
            ADXRNA_SetOutPan(handle->rna, 0, pan + default_pan[0]);
        }
        return;
    }

    pan = handle->output_pan[0];
    if (pan == -128 && default_pan[0] == -128) {
        ADXRNA_SetOutPan(handle->rna, 0, -15);
    } else if (pan != -128 && default_pan[0] == -128) {
        ADXRNA_SetOutPan(handle->rna, 0, pan);
    } else if (pan == -128 && default_pan[0] != -128) {
        ADXRNA_SetOutPan(handle->rna, 0, default_pan[0]);
    } else {
        ADXRNA_SetOutPan(handle->rna, 0, pan + default_pan[0]);
    }
    pan = handle->output_pan[1];
    if (pan == -128 && default_pan[1] == -128) {
        ADXRNA_SetOutPan(handle->rna, 1, 15);
    } else if (pan != -128 && default_pan[1] == -128) {
        ADXRNA_SetOutPan(handle->rna, 1, pan);
    } else if (pan == -128 && default_pan[1] != -128) {
        ADXRNA_SetOutPan(handle->rna, 1, default_pan[1]);
    } else {
        ADXRNA_SetOutPan(handle->rna, 1, pan + default_pan[1]);
    }
}

void adxt_eos_entry(void* object)
{
    ADXTHandle* handle = (ADXTHandle*)object;
    ADXStream* stream = handle->stream;
    AdxSjdHandle* decoder = handle->decoder;
    s32 loop_start_offset;

    if (stream == 0 || decoder == 0) {
        return;
    }
    loop_start_offset = ADXSJD_GetLpStartOfst(decoder);
    if (handle->stream_loop_enabled == 0) {
        ADXSJD_SetTrapNumSmpl(handle->decoder, -1);
        ADXSTM_SetEos(handle->stream, 0x7FFFFFFF);
    } else {
        ADXSTM_Seek(stream, loop_start_offset / ADXT_SECTOR_SIZE);
    }
}

void adxt_trap_entry(void* object)
{
    ADXTHandle* handle = (ADXTHandle*)object;
    AdxSjdHandle* decoder = handle->decoder;
    SJ* input = handle->input_sj;
    SJCK chunk;
    s32 loop_start_position;
    s32 loop_start_offset;
    s32 loop_end_position;

    loop_start_position = ADXSJD_GetLpStartPos(decoder);
    loop_start_offset = ADXSJD_GetLpStartOfst(decoder);
    loop_end_position = ADXSJD_GetLpEndPos(decoder);
    if ((handle->stream_type == ADXT_STREAM_TYPE_MEMORY ||
         handle->stream_type == ADXT_STREAM_TYPE_LINKED) &&
        handle->stream_loop_enabled == 0) {
        ADXSJD_SetTrapNumSmpl(handle->decoder, -1);
        return;
    }
    input->interface->get_chunk(
        input, 1, handle->link_data_length, &chunk);
    if (chunk.len < handle->link_data_length) {
        ADXERR_CallErrFunc1(adxt_loop_data_error);
    }
    input->interface->put_chunk(input, 0, &chunk);
    ADXSJD_SetTrapCnt(decoder, 0);
    handle->loop_sample_count = loop_end_position - loop_start_position;
    ADXSJD_SetTrapNumSmpl(decoder, handle->loop_sample_count);
    ADXSJD_SetTrapDtLen(decoder, loop_start_offset);
    ADXSJD_SetDecPos(decoder, loop_start_position);
    if (handle->stream_type == ADXT_STREAM_TYPE_MEMORY) {
        input->interface->reset(input);
        input->interface->get_chunk(
            input, 1, loop_start_offset, &chunk);
        input->interface->put_chunk(input, 0, &chunk);
    }
    ADXSJD_RestoreSnapshot(decoder);
    handle->loop_count++;
}

void adxt_trap_entry_lps(void* object)
{
    ADXTHandle* handle = (ADXTHandle*)object;
    AdxSjdHandle* decoder = handle->decoder;
    s32 loop_start_position;
    s32 loop_start_offset;
    s32 loop_end_position;

    loop_start_position = ADXSJD_GetLpStartPos(decoder);
    loop_start_offset = ADXSJD_GetLpStartOfst(decoder);
    loop_end_position = ADXSJD_GetLpEndPos(decoder);
    ADXSJD_TakeSnapshot(decoder);
    ADXSJD_SetTrapCnt(decoder, 0);
    handle->loop_sample_count = loop_end_position - loop_start_position;
    ADXSJD_SetTrapNumSmpl(decoder, handle->loop_sample_count);
    ADXSJD_SetTrapDtLen(decoder, loop_start_offset);
    ADXSJD_SetDecPos(decoder, loop_start_position);
    ADXSJD_EntryTrapFunc(decoder, adxt_trap_entry, handle);
}
