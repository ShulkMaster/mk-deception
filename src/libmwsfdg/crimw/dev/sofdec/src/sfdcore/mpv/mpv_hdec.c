#include "sofdec/mpv_mc.h"
#include "cri/mpv.h"
#include "cri/sj.h"
#include "runtime/cstdlib.h"
#include "runtime/cstring.h"
#include "sofdec/uty_mem.h"

extern u8* mpvvlc_y_dcsiz;
extern u8* mpvvlc_c_dcsiz;
extern u8* mpvvlc2_y_dcsiz;
extern u8* mpvvlc2_c_dcsiz;
extern int MPVABDEC_IntraBlock(void* context, void* block);
extern int MPVABDEC_IntraBlockDc11(void* context, void* block);
extern int MPVABDEC_NintraBlock(void* context, void* block);
int MPV_DecodePicAtrSj(MPVContext* context, SJ* stream);

typedef void (*MPVMacroblockDecodeFunction)(MPVContext* context, SJ* stream);
typedef void (*MPVMotionFunction)(MPVContext* context);
typedef void (*MPVSkipFunction)(MPVContext* context, int count);

extern void MPVDEC_DecIpicMb(MPVContext* context, SJ* stream);
extern void MPVDEC_DecPpicMb(MPVContext* context, SJ* stream);
extern void MPVDEC_DecBpicMb(MPVContext* context, SJ* stream);
extern void MPVDEC_DecDpicMb(MPVContext* context, SJ* stream);
extern void MPVUMC_PpicSkipped(MPVContext* context, int count);
extern void MPVUMC_BpicSkipped(MPVContext* context, int count);
extern void MPVUMC_Intra(MPVContext* context);
extern void MPVUMC_Forward(MPVContext* context);
extern void MPVUMC_Backward(MPVContext* context);
extern void MPVUMC_BiDirect(MPVContext* context);
extern int MPVCDEC_IntraBlocks(void* context);
extern int MPVCDEC_NintraBlocks(void* context);
extern u8 mpvbdec_dfl_iqm[64];
extern u8 mpvbdec_zigzag[64];
extern void MPVDEC_ResetDc(MPVContext* context);
extern void MPVDEC_ResetMv(MPVMotionInfo* motion);
extern int MPVLIB_CheckHn(MPVContext* context);
extern int MPVM2V_DecodePicAtr(MPVContext* context, SJ* stream);

#define MPVHDEC_READ_BITS(value, count)                                      \
    do {                                                                      \
        int split = 32 - (count);                                             \
        if (bit_offset >= split) {                                            \
            bit_offset -= split;                                              \
            if (bit_offset != 0) {                                            \
                bits |= next_bits >> ((count) - bit_offset);                  \
                (value) = bits >> split;                                      \
                bits = next_bits << bit_offset;                               \
            } else {                                                          \
                (value) = bits >> split;                                      \
                bits = next_bits;                                             \
            }                                                                 \
            next_bits = *words++;                                             \
        } else {                                                              \
            (value) = bits >> split;                                          \
            bit_offset += (count);                                            \
            bits <<= (count);                                                 \
        }                                                                     \
    } while (0)

#define MPVHDEC_READ_FLAG(value)                                              \
    do {                                                                      \
        (value) = bits >> 31;                                                  \
        if (bit_offset == 31) {                                               \
            bits = next_bits;                                                 \
            next_bits = *words++;                                             \
            bit_offset = 0;                                                   \
        } else {                                                              \
            bits <<= 1;                                                       \
            bit_offset++;                                                     \
        }                                                                     \
    } while (0)

static const MPVMacroblockDecodeFunction dec_mbs_func[5] = {
    0,
    MPVDEC_DecIpicMb,
    MPVDEC_DecPpicMb,
    MPVDEC_DecBpicMb,
    MPVDEC_DecDpicMb,
};

static MPVMotionFunction mc_bidirect_func[10];
static MPVMotionFunction mc_backward_func[10];
static MPVMotionFunction mc_forward_func[10];
static MPVMotionFunction mc_intra_func[20];
static MPVSkipFunction skip_func[10];

static void mpvhdec_DecSlice(MPVContext* context, SJ* stream)
{
    SJCK remainder;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 first_bits;
    u32 value;
    int bit_offset;
    int consumed;

    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);
    words = (const u32*)((u32)context->header_chunk.data & ~3);
    bit_offset = (context->header_chunk.data - (const u8*)words) * 8;
    first_bits = words[0] << bit_offset;
    next_bits = words[1];
    if (bit_offset != 0) {
        bits = next_bits << bit_offset;
        value = first_bits | (next_bits >> (32 - bit_offset));
    } else {
        bits = next_bits;
        value = first_bits;
    }
    next_bits = words[2];
    words += 3;

    value = (u8)value - 1;
    context->macroblock_index =
        value * context->condition_state.decoder.picture.macroblocks_per_row - 1;
    context->macroblock_row = value;
    context->macroblock_column = -1;

    MPVHDEC_READ_BITS(context->quantizer_scale, 5);
    MPVDEC_ResetMv(&context->forward_motion);
    MPVDEC_ResetMv(&context->backward_motion);
    MPVDEC_ResetDc(context);

    for (;;) {
        if ((bits >> 31) == 0) {
            bit_offset++;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                words++;
            }
            break;
        }
        bit_offset += 9;
        if (bit_offset >= 32) {
            bit_offset -= 32;
            bits = next_bits << bit_offset;
            next_bits = *words++;
        } else {
            bits <<= 9;
        }
        consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
                   context->header_chunk.data;
        if (context->header_chunk.len <= consumed) {
            return;
        }
    }

    context->field_1310 = bit_offset & 7;
    consumed = ((const u8*)words +
                ((bit_offset - context->field_1310 + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &remainder);
    context->decode_macroblock(context, stream);
}

int MPV_MoveChunk(SJ* stream, int channel, int size)
{
    SJCK chunk;
    int source_channel = channel;

    channel = channel == 0;
    stream->interface->get_chunk(stream, source_channel, size, &chunk);
    stream->interface->put_chunk(stream, channel, &chunk);
    return chunk.len;
}

static inline int mpvhdec_SeekDelimSj(SJ* stream, int wanted_mask)
{
    SJCK chunk;
    SJCK remainder;
    const u8* delimiter;
    int delimiter_type;

    for (;;) {
        stream->interface->get_chunk(stream, 1, 0x7FFFFFFF, &chunk);
        if (chunk.len < 4) {
            stream->interface->unget_chunk(stream, 1, &chunk);
            return 0;
        }

        delimiter = MPV_SearchDelim(chunk.data, chunk.len, -1);
        if (delimiter == 0) {
            SJ_SplitChunk(&chunk, chunk.len - 3, &chunk, &remainder);
            stream->interface->put_chunk(stream, 0, &chunk);
            stream->interface->unget_chunk(stream, 1, &remainder);
            continue;
        }

        delimiter_type = MPV_CheckDelim(delimiter);
        SJ_SplitChunk(
            &chunk, delimiter - chunk.data, &chunk, &remainder);
        stream->interface->put_chunk(stream, 0, &chunk);
        stream->interface->unget_chunk(stream, 1, &remainder);
        if ((delimiter_type & wanted_mask) != 0) {
            return delimiter_type;
        }
        if (MPV_MoveChunk(stream, 1, 4) != 4) {
            return 0;
        }
    }
}

int MPVHDEC_DecPicture(MPVContext* context, SJ* stream)
{
    SJCK chunk;
    int delimiter_type;
    int error;
    int picture_state;

    context->field_1324 = context->condition_state.decoder.field_1AC;
    for (;;) {
        do {
            picture_state = context->condition_state.conditions[1];
            if (context->field_1300 != 0) {
                context->field_1300 = 0;
                context->field_1304++;
                context->error_info.field_0C++;
                if (picture_state == 0) {
                    error = -2;
                    break;
                }
                context->error_info.field_10++;
            }

            if (picture_state == 0) {
                error = -2;
            } else {
                error = -3;
            }

            delimiter_type = mpvhdec_SeekDelimSj(stream, -1);

            if (delimiter_type != 0) {
                error = 0;
            }
        } while (0);

        if (error != 0) {
            return MPVERR_SetCode(context, error);
        }

        stream->interface->get_chunk(stream, 1, 0x7FFFFFFF, &chunk);
        stream->interface->unget_chunk(stream, 1, &chunk);
        if (chunk.len >= 4 && (MPV_CheckDelim(chunk.data) & 1) != 0) {
            mpvhdec_DecSlice(context, stream);
        } else {
            return 0;
        }
    }
}

int MPV_GoNextDelimSj(SJ* stream)
{
    SJCK chunk;
    SJCK remainder;
    const u8* delimiter;
    int delimiter_type;

    for (;;) {
        stream->interface->get_chunk(stream, 1, 0x7FFFFFFF, &chunk);
        if (chunk.len < 4) {
            stream->interface->unget_chunk(stream, 1, &chunk);
            return 0;
        }

        delimiter = MPV_SearchDelim(chunk.data, chunk.len, -1);
        if (delimiter == 0) {
            SJ_SplitChunk(&chunk, chunk.len - 3, &chunk, &remainder);
            stream->interface->put_chunk(stream, 0, &chunk);
            stream->interface->unget_chunk(stream, 1, &remainder);
            continue;
        }

        delimiter_type = MPV_CheckDelim(delimiter);
        SJ_SplitChunk(&chunk, delimiter - chunk.data, &chunk, &remainder);
        stream->interface->put_chunk(stream, 0, &chunk);
        stream->interface->unget_chunk(stream, 1, &remainder);
        return delimiter_type;
    }
}

static inline void mpvhdec_ConsumeDelim(MPVContext* context, SJ* stream)
{
    SJCK remainder;
    u8* aligned;
    int bit_offset;
    int consumed;

    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);
    aligned = (u8*)((u32)context->header_chunk.data & ~3);
    bit_offset = (context->header_chunk.data - aligned) * 8;
    consumed = (aligned + ((bit_offset + 7) >> 3) + 4) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &remainder);
    MPV_GoNextDelimSj(stream);
}

static int mpvhdec_DecSeqUdsc(MPVContext* context, const u8* data, int length)
{
    const u8* current;
    int offset;
    int result;

    result = 0;
    offset = 0;
    while (offset < length - 4) {
        current = data + offset + 4;
        if (strncmp((const char*)current, "IDCPREC", 7) == 0) {
            if (atoi((const char*)current + 0x10) == 0) {
                context->field_1314 = 0;
            } else {
                context->field_1314 = 3;
            }
        }
        if (strncmp((const char*)current, "STCCODE", 7) == 0) {
            context->stc_code_0 = atoi((const char*)current + 0x10);
            context->stc_code_1 = atoi((const char*)current + 0x18);
            context->stc_code_2 = atoi((const char*)current + 0x20);
        }
        if (MPV_CheckDelim(current) != 0) {
            break;
        }
        offset++;
    }

    if (context->field_1314 == 0) {
        context->decode_intra_block = MPVABDEC_IntraBlock;
        context->y_dc_size = mpvvlc_y_dcsiz;
        context->chroma_dc_size = mpvvlc_c_dcsiz;
    } else {
        context->decode_intra_block = MPVABDEC_IntraBlockDc11;
        context->y_dc_size = mpvvlc2_y_dcsiz;
        context->chroma_dc_size = mpvvlc2_c_dcsiz;
    }

    if (context->stc_code_0 == 8) {
        result = -1;
    } else {
        if (context->field_1314 == 0) {
            context->decode_intra_block = MPVABDEC_IntraBlock;
        } else {
            context->decode_intra_block = MPVABDEC_IntraBlockDc11;
        }
        context->decode_nonintra_block = MPVABDEC_NintraBlock;
    }
    return result;
}

static int mpvhdec_AnalyUd(MPVContext* context, const u8* data, int length)
{
    SJ* user_sj;
    SJCK first;
    SJCK second;
    int delimiter_offset;
    int delimiter_result;
    int sequence_result;
    int index;
    int copy_size;
    int picture_copy_size;

    sequence_result = 0;
    delimiter_result = 0;
    delimiter_offset = 4;
    index = context->user_data_index;
    while (delimiter_offset < length - 3) {
        if (MPV_CheckDelim(data + delimiter_offset) != 0) {
            break;
        }
        delimiter_offset++;
    }
    if (delimiter_offset == length - 3) {
        delimiter_result = -1;
    }
    copy_size = delimiter_offset;
    if (index == 1) {
        sequence_result =
            mpvhdec_DecSeqUdsc(context, data, delimiter_offset);
    }

    user_sj = context->user_streams[index].stream;
    if (user_sj != 0) {
        user_sj->interface->get_chunk(user_sj, 0, copy_size, &first);
        memcpy(first.data, data, first.len);
        user_sj->interface->put_chunk(user_sj, 1, &first);
        if (first.len < copy_size) {
            user_sj->interface->get_chunk(
                user_sj, 0, copy_size - first.len, &second);
            memcpy(second.data, data + first.len, second.len);
            user_sj->interface->put_chunk(user_sj, 1, &second);
        }
        if (context->user_streams[index].callback != 0) {
            context->user_streams[index].callback(
                context->user_streams[index].callback_argument, index);
        }
    }

    if (index == 3 && context->picture_user.buffer != 0) {
        picture_copy_size = context->picture_user.capacity;
        if (copy_size < picture_copy_size) {
            picture_copy_size = copy_size;
        }
        context->picture_user.size = picture_copy_size;
        memcpy(context->picture_user.buffer, data, picture_copy_size);
    }

    if (sequence_result != 0) {
        return sequence_result;
    }
    return delimiter_result;
}

static int mpvhdec_DecPscSj(MPVContext* context, SJ* stream)
{
    SJCK remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 value;
    int bit_offset;
    int f_code;
    int picture_type;
    int table_index;
    int condition_index;
    int motion_mode;
    int consumed;

    context->user_data_index = 3;
    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);

    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    bit_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    bits = aligned[1];
    if (bit_offset != 0) {
        bits <<= bit_offset;
    }
    next_bits = aligned[2];
    words = aligned + 3;

    MPVHDEC_READ_BITS(
        context->condition_state.decoder.picture.temporal_reference, 10);
    MPVHDEC_READ_BITS(
        context->condition_state.decoder.picture.picture_type, 3);
    picture_type = context->condition_state.decoder.picture.picture_type;
    MPVHDEC_READ_BITS(context->vbv_delay, 16);

    if (picture_type == 2 || picture_type == 3) {
        MPVHDEC_READ_FLAG(context->forward_motion.full_pel);
        MPVHDEC_READ_BITS(value, 3);
        f_code = value - 1;
        context->forward_motion.r_size = f_code;
        context->forward_motion.shift = 27 - f_code;
        context->forward_motion.limit = 1 << f_code;
    }
    if (picture_type == 3) {
        MPVHDEC_READ_FLAG(context->backward_motion.full_pel);
        MPVHDEC_READ_BITS(value, 3);
        f_code = value - 1;
        context->backward_motion.r_size = f_code;
        context->backward_motion.shift = 27 - f_code;
        context->backward_motion.limit = 1 << f_code;
    }

    condition_index = context->condition_state.decoder.field_1A8 != 3;
    table_index = condition_index * 5 + picture_type;
    motion_mode = context->condition_state.decoder.field_1A0;
    context->decode_intra_blocks = MPVCDEC_IntraBlocks;
    context->decode_nonintra_blocks = MPVCDEC_NintraBlocks;
    context->decode_macroblock = dec_mbs_func[picture_type];
    context->skip_macroblocks = skip_func[table_index];
    context->motion_intra =
        mc_intra_func[motion_mode * 10 + table_index];
    context->motion_backward = mc_backward_func[table_index];
    context->motion_forward = mc_forward_func[table_index];
    context->motion_bidirect = mc_bidirect_func[table_index];
    context->motion_skipped = context->motion_forward;

    for (;;) {
        if ((bits >> 31) == 0) {
            bit_offset++;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                words++;
            }
            break;
        }

        bit_offset += 9;
        if (bit_offset >= 32) {
            bit_offset -= 32;
            bits = next_bits << bit_offset;
            next_bits = *words++;
        } else {
            bits <<= 9;
        }
        consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
                   context->header_chunk.data;
        if (context->header_chunk.len <= consumed) {
            return -3;
        }
    }

    consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &remainder);
    return 0;
}

static int mpvhdec_DecGscSj(MPVContext* context, SJ* stream)
{
    SJCK remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 value;
    int bit_offset;
    int split;
    int consumed;

    context->user_data_index = 2;
    context->condition_state.decoder.picture.group_count++;
    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);

    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    bit_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    bits = aligned[1];
    if (bit_offset != 0) {
        bits <<= bit_offset;
    }
    next_bits = aligned[2];
    words = aligned + 3;

    if (bit_offset >= 7) {
        bit_offset -= 7;
        if (bit_offset != 0) {
            bits |= next_bits >> (25 - bit_offset);
            value = bits >> 7;
            bits = next_bits << bit_offset;
        } else {
            value = bits >> 7;
            bits = next_bits;
        }
        next_bits = *words++;
    } else {
        value = bits >> 7;
        bits <<= 25;
        bit_offset += 25;
    }

    context->condition_state.decoder.picture.time_code_pictures =
        value & 0x3F;
    context->condition_state.decoder.picture.time_code_seconds =
        (value >> 6) & 0x3F;
    context->condition_state.decoder.picture.time_code_minutes =
        (value >> 13) & 0x3F;
    context->condition_state.decoder.picture.time_code_hours =
        (value >> 19) & 0x1F;
    context->condition_state.decoder.picture.drop_frame_flag = value >> 24;

    context->link_flag_0 = bits >> 31;
    if (bit_offset == 31) {
        bits = next_bits;
        bit_offset = 0;
        words++;
    } else {
        bits <<= 1;
        bit_offset++;
    }

    context->link_flag_1 = bits >> 31;
    if (bit_offset == 31) {
        bit_offset = 0;
        words++;
    } else {
        bit_offset++;
    }

    split = (bit_offset + 7) >> 3;
    consumed = ((const u8*)words + split - 8) - context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &remainder);
    return 0;
}

static int mpvhdec_DecShcSj(MPVContext* context, SJ* stream)
{
    SJCK remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 value;
    int bit_offset;
    int index;
    int consumed;

    context->user_data_index = 1;
    context->condition_state.decoder.picture.field_34++;
    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);

    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    bit_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    bits = aligned[1];
    if (bit_offset != 0) {
        bits <<= bit_offset;
    }
    next_bits = aligned[2];
    words = aligned + 3;

    MPVHDEC_READ_BITS(context->condition_state.decoder.picture.width, 12);
    MPVHDEC_READ_BITS(context->condition_state.decoder.picture.height, 12);
    MPVHDEC_READ_BITS(context->field_2A4, 4);
    MPVHDEC_READ_BITS(
        context->condition_state.decoder.picture.frame_rate_code, 4);
    MPVHDEC_READ_BITS(context->bit_rate, 18);
    MPVHDEC_READ_FLAG(value);
    MPVHDEC_READ_BITS(context->vbv_buffer_units, 10);
    MPVHDEC_READ_FLAG(context->field_2B0);

    MPVHDEC_READ_FLAG(value);
    if (value != 0) {
        for (index = 0; index < 64; index++) {
            MPVHDEC_READ_BITS(value, 8);
            context->intra_quant_matrix[(s8)mpvbdec_zigzag[index]] = value;
        }
    } else {
        UTY_MemcpyDword((unsigned int*)context->intra_quant_matrix,
                        (const unsigned int*)mpvbdec_dfl_iqm, 16);
    }

    MPVHDEC_READ_FLAG(value);
    if (value != 0) {
        for (index = 0; index < 64; index++) {
            MPVHDEC_READ_BITS(value, 8);
            context->nonintra_quant_matrix[(s8)mpvbdec_zigzag[index]] = value;
        }
    } else {
        UTY_MemsetDword((unsigned int*)context->nonintra_quant_matrix,
                        0x10101010, 16);
    }

    context->condition_state.decoder.picture.macroblocks_per_row =
        (context->condition_state.decoder.picture.width + 15) >> 4;
    context->condition_state.decoder.picture.macroblock_rows =
        (context->condition_state.decoder.picture.height + 15) >> 4;
    context->last_macroblock_index =
        context->condition_state.decoder.picture.macroblock_rows *
        context->condition_state.decoder.picture.macroblocks_per_row - 1;
    context->condition_state.decoder.picture.field_48 = context->bit_rate;
    context->condition_state.decoder.picture.field_4C =
        context->vbv_buffer_units;
    context->condition_state.decoder.picture.field_59 = context->field_2A4;
    context->condition_state.decoder.picture.field_5A = context->field_2B0;

    consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &remainder);
    return 0;
}

int MPV_DecodePicAtr(MPVContext* context, const SJCK* input,
                     int* consumed_size)
{
    SJ* stream;
    int result;

    stream = SJMEM_Create(input->data, input->len);
    if (stream == 0) {
        return -1;
    }
    result = MPV_DecodePicAtrSj(context, stream);
    *consumed_size =
        input->len - stream->interface->get_num_data(stream, 1);
    stream->interface->destroy(stream);
    return result;
}

int MPV_DecodePicAtrSj(MPVContext* context, SJ* stream)
{
    SJCK chunk;
    SJCK remainder;
    const u8* delimiter;
    const u8* next;
    const u8* chunk_data;
    u8* aligned;
    int chunk_len;
    int bit_offset;
    int consumed;
    int delimiter_type;
    int error;
    int decoder_mode;

    if (MPVLIB_CheckHn(context) != 0) {
        return MPVERR_SetCode(0, 0xFF03020C);
    }
    context->picture_user.size = 0;

    stream->interface->get_chunk(stream, 1, 0x7FFFFFFF, &chunk);
    stream->interface->unget_chunk(stream, 1, &chunk);
    chunk_data = chunk.data;
    chunk_len = chunk.len;
    decoder_mode = context->field_358;
    if (decoder_mode == 0) {
        delimiter = MPV_SearchDelim(chunk_data, chunk_len, 0x40);
        if (delimiter != 0) {
            next = MPV_SearchDelim(
                delimiter + 4,
                chunk_len - ((delimiter + 4) - chunk_data), -1);
            if (next != 0) {
                delimiter_type = MPV_CheckDelim(next);
                if (delimiter_type & 0x10) {
                    context->field_358 = 2;
                } else if (delimiter_type != 0) {
                    context->field_358 = 1;
                }
            }
        }
        decoder_mode = context->field_358;
    }
    if (decoder_mode == 2) {
        return MPVM2V_DecodePicAtr(context, stream);
    }

    for (;;) {
        error = context->condition_state.conditions[1] == 0 ? -2 : -3;
        if (context->field_1300 != 0) {
            context->field_1300 = 0;
            context->field_1304++;
            context->error_info.field_0C++;
            if (context->condition_state.conditions[1] == 0) {
                return MPVERR_SetCode(context, -2);
            }
            context->error_info.field_10++;
        }

        delimiter_type = mpvhdec_SeekDelimSj(stream, -1);
        if (delimiter_type == 0) {
            return MPVERR_SetCode(context, error);
        }

        stream->interface->get_chunk(stream, 1, 0x7FFFFFFF, &chunk);
        stream->interface->unget_chunk(stream, 1, &chunk);
        delimiter_type = chunk.len < 4 ? 0 : MPV_CheckDelim(chunk.data);
        if (delimiter_type == 0 || (delimiter_type & 3) != 0) {
            return 0;
        }

        switch (delimiter_type) {
        case 0x40:
            mpvhdec_DecShcSj(context, stream);
            break;
        case 8:
            mpvhdec_DecGscSj(context, stream);
            break;
        case 4:
            mpvhdec_DecPscSj(context, stream);
            break;
        case 0x10:
            mpvhdec_ConsumeDelim(context, stream);
            break;
        case 0x20:
            stream->interface->get_chunk(
                stream, 1, 0x7FFFFFFF, &context->header_chunk);
            mpvhdec_AnalyUd(
                context, context->header_chunk.data, context->header_chunk.len);
            aligned = (u8*)((u32)context->header_chunk.data & ~3);
            bit_offset = (context->header_chunk.data - aligned) * 8;
            consumed = (aligned + ((bit_offset + 7) >> 3) + 4) -
                       context->header_chunk.data;
            SJ_SplitChunk(&context->header_chunk, consumed,
                          &context->header_chunk, &remainder);
            stream->interface->put_chunk(stream, 0, &context->header_chunk);
            stream->interface->unget_chunk(stream, 1, &remainder);
            MPV_GoNextDelimSj(stream);
            break;
        }
    }
}

void MPV_GetPicUsr(MPVContext* context, void** buffer, int* size)
{
    if (buffer != 0) {
        *buffer = context->picture_user.buffer;
    }
    if (size != 0) {
        *size = context->picture_user.size;
    }
}

void MPV_SetPicUsrBuf(MPVContext* context, void* buffer, int capacity)
{
    context->picture_user.buffer = buffer;
    context->picture_user.capacity = capacity;
    context->picture_user.size = 0;
}

void MPV_SetUsrSj(MPVContext* context, int index, void* stream,
                  void (*callback)(void* argument, int index),
                  void* callback_argument)
{
    MPVUserStream* user_stream = &context->user_streams[index];

    user_stream->stream = stream;
    user_stream->callback = callback;
    user_stream->callback_argument = callback_argument;
}

void MPVHDEC_Init(void)
{
    memset(skip_func, 0, sizeof(skip_func));
    memset(mc_intra_func, 0, sizeof(mc_intra_func));
    memset(mc_forward_func, 0, sizeof(mc_forward_func));
    memset(mc_backward_func, 0, sizeof(mc_backward_func));
    memset(mc_bidirect_func, 0, sizeof(mc_bidirect_func));

    skip_func[2] = MPVUMC_PpicSkipped;
    skip_func[3] = MPVUMC_BpicSkipped;
    mc_intra_func[1] = MPVUMC_Intra;
    mc_intra_func[2] = MPVUMC_Intra;
    mc_intra_func[3] = MPVUMC_Intra;
    mc_intra_func[4] = MPVUMC_Intra;
    mc_intra_func[11] = MPVUMC_Intra;
    mc_intra_func[12] = MPVUMC_Intra;
    mc_intra_func[13] = MPVUMC_Intra;
    mc_intra_func[14] = MPVUMC_Intra;
    mc_forward_func[2] = MPVUMC_Forward;
    mc_forward_func[3] = MPVUMC_Forward;
    mc_backward_func[3] = MPVUMC_Backward;
    mc_bidirect_func[3] = MPVUMC_BiDirect;
}

const u32 gap_04_80317D44_rodata = 0;
