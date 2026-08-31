#include "sofdec/mpv_mc.h"
#include "runtime/cstring.h"

extern s16* mpvvlc_motion_0;
extern s16* mpvvlc_motion_1;
extern s16* mpvvlc_mbai_i_0;
extern s16* mpvvlc_mbai_i_1;
extern s16* mpvvlc_mbai_p_0;
extern s16* mpvvlc_mbai_p_1;
extern s16* mpvvlc_mbai_b_0;
extern s16* mpvvlc_mbai_b_1;
extern s16* mpvvlc_p_mbtype;
extern s16* mpvvlc_b_mbtype;
extern s16* mpvvlc_cbp;
extern int MPV_GoNextDelimSj(SJ* stream);

static int mpvdec_MotionSub(MPVBitReader* reader, MPVMotionInfo* motion,
                            s32* output, s32* predictor);

static inline void mpvdec_InitReader(MPVBitReader* reader,
                                     const u8* data, int extra_offset)
{
    const u32* aligned = (const u32*)((u32)data & ~3);
    int byte_offset = (data - (const u8*)aligned) * 8;

    reader->next_bits = aligned[1];
    reader->words = aligned + 2;
    reader->bits = aligned[0] << byte_offset;
    reader->bit_offset = byte_offset + extra_offset;
    if (reader->bit_offset >= 32) {
        reader->bit_offset -= 32;
        reader->bits = reader->next_bits << reader->bit_offset;
        reader->next_bits = *reader->words++;
    } else {
        reader->bits <<= extra_offset;
    }
}

static inline u32 mpvdec_PeekBits(const MPVBitReader* reader, int count)
{
    u32 value = reader->bits >> (32 - count);

    if (reader->bit_offset > 32 - count) {
        value |= reader->next_bits >>
                 (64 - count - reader->bit_offset);
    }
    return value;
}

static inline void mpvdec_FlushBits(MPVBitReader* reader, int count)
{
    reader->bit_offset += count;
    if (reader->bit_offset >= 32) {
        reader->bit_offset -= 32;
        reader->bits = reader->next_bits << reader->bit_offset;
        reader->next_bits = *reader->words++;
    } else {
        reader->bits <<= count;
    }
}

static inline u32 mpvdec_DecodeMbAddressI(MPVContext* context,
                                          MPVBitReader* reader)
{
    int old_index = context->macroblock_index;

    for (;;) {
        u32 peek = mpvdec_PeekBits(reader, 20);
        int descriptor;
        u8 encoded_increment;
        int increment;
        u32 delta;

        if ((peek >> 8) == 0) {
            descriptor = mpvvlc_mbai_i_0[peek];
        } else {
            descriptor = mpvvlc_mbai_i_1[peek >> 6];
        }
        mpvdec_FlushBits(reader, descriptor & 0xF);
        encoded_increment = descriptor >> 2;
        increment = encoded_increment >> 2;
        if (increment == 34) {
            continue;
        }
        if (increment == 35) {
            context->macroblock_index += 33;
            continue;
        }
        if (increment == 36) {
            return (u32)-2;
        }

        context->macroblock_index += increment;
        context->field_344 = (u32)descriptor >> 10;
        if (context->macroblock_index > context->last_macroblock_index) {
            return (u32)-2;
        }

        delta = context->macroblock_index - old_index;
        context->macroblock_column += delta;
        while (context->macroblock_column >=
               context->condition_state.decoder.picture.macroblocks_per_row) {
            context->macroblock_column -=
                context->condition_state.decoder.picture.macroblocks_per_row;
            context->macroblock_row++;
        }
        return delta;
    }
}

void MPVDEC_DecDpicMb(MPVContext* context, SJ* stream)
{
    SJCK refill_remainder;
    SJCK final_remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 peek;
    u32 delta;
    u32 marker;
    int bit_offset;
    int byte_offset;
    int residual_offset;
    int consumed;

    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);
    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    byte_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    next_bits = aligned[1];
    words = aligned + 2;
    bits = aligned[0] << byte_offset;
    bit_offset = byte_offset + context->field_1310;
    if (bit_offset >= 32) {
        bit_offset -= 32;
        bits = next_bits << bit_offset;
        next_bits = *words++;
    } else {
        bits <<= context->field_1310;
    }

    for (;;) {
        int old_index;

        peek = bits >> 9;
        if (bit_offset > 9) {
            peek |= next_bits >> (41 - bit_offset);
        }
        if (peek == 0) {
            break;
        }

        old_index = context->macroblock_index;
        for (;;) {
            int descriptor;
            int code_length;
            u8 encoded_increment;
            int increment;

            peek = bits >> 20;
            if (bit_offset > 20) {
                peek |= next_bits >> (52 - bit_offset);
            }
            if ((peek >> 8) == 0) {
                descriptor = mpvvlc_mbai_i_0[peek];
            } else {
                descriptor = mpvvlc_mbai_i_1[peek >> 6];
            }

            code_length = descriptor & 0xF;
            bit_offset += code_length;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }

            encoded_increment = descriptor >> 2;
            increment = encoded_increment >> 2;
            if (increment == 34) {
                continue;
            }
            if (increment == 35) {
                context->macroblock_index += 33;
                continue;
            }
            if (increment == 36) {
                delta = (u32)-2;
            } else {
                context->macroblock_index += increment;
                context->field_344 = (u32)descriptor >> 10;
                if (context->macroblock_index >
                    context->last_macroblock_index) {
                    delta = (u32)-2;
                } else {
                    delta = context->macroblock_index - old_index;
                    context->macroblock_column += delta;
                    while (context->macroblock_column >=
                           context->condition_state.decoder.picture.
                               macroblocks_per_row) {
                        context->macroblock_column -=
                            context->condition_state.decoder.picture.
                                macroblocks_per_row;
                        context->macroblock_row++;
                    }
                }
            }
            break;
        }

        if (delta == (u32)-2) {
            break;
        }

        context->bit_reader.bits = bits;
        context->bit_reader.next_bits = next_bits;
        context->bit_reader.bit_offset = bit_offset;
        context->bit_reader.words = words;
        context->decode_intra_blocks(context);
        context->motion_intra(context);
        if (--context->field_1324 <= 0) {
            context->field_1324 = context->condition_state.decoder.field_1AC;
            context->condition_state.decoder.callback(
                context->condition_state.decoder.callback_argument);
        }
        bits = context->bit_reader.bits;
        next_bits = context->bit_reader.next_bits;
        bit_offset = context->bit_reader.bit_offset;
        words = context->bit_reader.words;

        marker = bits >> 31;
        if (bit_offset == 31) {
            bits = next_bits;
            next_bits = *words++;
            bit_offset = 0;
        } else {
            bits <<= 1;
            bit_offset++;
        }
        if (marker != 1) {
            break;
        }

        residual_offset = bit_offset & 7;
        consumed = ((const u8*)words +
                    ((bit_offset - residual_offset + 7) >> 3) - 8) -
                   context->header_chunk.data;
        if (context->header_chunk.len - consumed <= 0x800) {
            SJ_SplitChunk(&context->header_chunk, consumed,
                          &context->header_chunk, &refill_remainder);
            stream->interface->put_chunk(
                stream, 0, &context->header_chunk);
            stream->interface->unget_chunk(stream, 1, &refill_remainder);
            stream->interface->get_chunk(
                stream, 1, 0x7FFFFFFF, &context->header_chunk);
            aligned =
                (const u32*)((u32)context->header_chunk.data & ~3);
            byte_offset =
                (context->header_chunk.data - (const u8*)aligned) * 8;
            next_bits = aligned[1];
            words = aligned + 2;
            bits = aligned[0] << byte_offset;
            bit_offset = byte_offset + residual_offset;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= residual_offset;
            }
        }
    }

    consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &final_remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &final_remainder);
    MPV_GoNextDelimSj(stream);
}

void MPVDEC_DecBpicMb(MPVContext* context, SJ* stream)
{
    SJCK refill_remainder;
    SJCK final_remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 peek;
    u32 delta;
    u32 quantizer;
    int bit_offset;
    int byte_offset;
    int residual_offset;
    int consumed;
    int first_macroblock = 1;

    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);
    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    byte_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    bits = aligned[0] << byte_offset;
    next_bits = aligned[1];
    words = aligned + 2;
    bit_offset = byte_offset + context->field_1310;
    if (bit_offset >= 32) {
        bit_offset -= 32;
        bits = next_bits << bit_offset;
        next_bits = *words++;
    } else {
        bits <<= context->field_1310;
    }

    for (;;) {
        int old_index;

        peek = bits >> 9;
        if (bit_offset > 9) {
            peek |= next_bits >> (41 - bit_offset);
        }
        if (peek == 0) {
            break;
        }

        old_index = context->macroblock_index;
        for (;;) {
            int descriptor;
            u8 code_length;
            u8 encoded_increment;
            int increment;

            peek = bits >> 21;
            if (bit_offset > 21) {
                peek |= next_bits >> (53 - bit_offset);
            }
            if ((peek >> 7) == 0) {
                descriptor = mpvvlc_mbai_b_0[peek];
            } else {
                descriptor = mpvvlc_mbai_b_1[peek >> 6];
            }

            code_length = descriptor & 0xF;
            bit_offset += code_length;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }

            encoded_increment = (u32)descriptor >> 2;
            increment = encoded_increment >> 2;
            if (increment == 34) {
                continue;
            }
            if (increment == 35) {
                context->macroblock_index += 33;
                continue;
            }
            if (increment == 36) {
                delta = (u32)-2;
            } else {
                context->macroblock_index += increment;
                context->field_344 = (u32)descriptor >> 10;
                if (context->macroblock_index >
                    context->last_macroblock_index) {
                    delta = (u32)-2;
                } else {
                    delta = context->macroblock_index - old_index;
                    context->macroblock_column += delta;
                    while (context->macroblock_column >=
                           context->condition_state.decoder.picture.
                               macroblocks_per_row) {
                        context->macroblock_column -=
                            context->condition_state.decoder.picture.
                                macroblocks_per_row;
                        context->macroblock_row++;
                    }
                }
            }
            break;
        }

        if (delta == (u32)-2) {
            break;
        }

        if (!first_macroblock && delta > 1) {
            context->skip_macroblocks(context, delta);
            context->dc_predictor_y = 0x400;
            context->dc_predictor_cr = 0x400;
            context->dc_predictor_cb = 0x400;
        }

        if ((context->field_344 & 0x20) == 0) {
            s16 descriptor;
            u8 code_length;

            peek = bits >> 26;
            if (bit_offset > 26) {
                peek |= next_bits >> (58 - bit_offset);
            }
            descriptor = mpvvlc_b_mbtype[peek];
            code_length = descriptor;
            bit_offset += code_length;
            context->field_344 = (u32)descriptor >> 8;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }
        }

        if ((context->field_344 & 0x10) != 0) {
            if (bit_offset >= 27) {
                bit_offset -= 27;
                if (bit_offset != 0) {
                    quantizer =
                        (bits | (next_bits >> (5 - bit_offset))) >> 27;
                    bits = next_bits << bit_offset;
                } else {
                    quantizer = bits >> 27;
                    bits = next_bits;
                }
                next_bits = *words++;
            } else {
                quantizer = bits >> 27;
                bits <<= 5;
                bit_offset += 5;
            }
            context->quantizer_scale = quantizer;
        }

        if ((context->field_344 & 8) != 0) {
            int horizontal_result;
            int vertical_result;

            context->bit_reader.bits = bits;
            context->bit_reader.next_bits = next_bits;
            context->bit_reader.bit_offset = bit_offset;
            context->bit_reader.words = words;
            horizontal_result = mpvdec_MotionSub(
                &context->bit_reader, &context->forward_motion,
                &context->forward_motion.horizontal,
                &context->forward_motion.previous_horizontal);
            vertical_result = mpvdec_MotionSub(
                &context->bit_reader, &context->forward_motion,
                &context->forward_motion.vertical,
                &context->forward_motion.previous_vertical);
            bits = context->bit_reader.bits;
            next_bits = context->bit_reader.next_bits;
            bit_offset = context->bit_reader.bit_offset;
            words = context->bit_reader.words;
            if ((horizontal_result | vertical_result) != 0) {
                break;
            }
        }

        if ((context->field_344 & 4) != 0) {
            int horizontal_result;
            int vertical_result;

            context->bit_reader.bits = bits;
            context->bit_reader.next_bits = next_bits;
            context->bit_reader.bit_offset = bit_offset;
            context->bit_reader.words = words;
            horizontal_result = mpvdec_MotionSub(
                &context->bit_reader, &context->backward_motion,
                &context->backward_motion.horizontal,
                &context->backward_motion.previous_horizontal);
            vertical_result = mpvdec_MotionSub(
                &context->bit_reader, &context->backward_motion,
                &context->backward_motion.vertical,
                &context->backward_motion.previous_vertical);
            bits = context->bit_reader.bits;
            next_bits = context->bit_reader.next_bits;
            bit_offset = context->bit_reader.bit_offset;
            words = context->bit_reader.words;
            if ((horizontal_result | vertical_result) != 0) {
                break;
            }
        }

        if ((context->field_344 & 2) != 0) {
            s16 descriptor;
            u8 code_length;

            peek = bits >> 23;
            if (bit_offset > 23) {
                peek |= next_bits >> (55 - bit_offset);
            }
            descriptor = mpvvlc_cbp[peek];
            code_length = descriptor;
            bit_offset += code_length;
            context->cbp_mask = ((u32)(u16)descriptor << 16) & 0xFFF00000;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }
        } else {
            context->cbp_mask = 0;
        }

        context->bit_reader.bits = bits;
        context->bit_reader.next_bits = next_bits;
        context->bit_reader.bit_offset = bit_offset;
        context->bit_reader.words = words;
        if ((context->field_344 & 1) != 0) {
            context->decode_intra_blocks(context);
            context->motion_intra(context);
            context->forward_motion.previous_horizontal = 0;
            context->forward_motion.previous_vertical = 0;
            context->forward_motion.horizontal = 0;
            context->forward_motion.vertical = 0;
            context->backward_motion.previous_horizontal = 0;
            context->backward_motion.previous_vertical = 0;
            context->backward_motion.horizontal = 0;
            context->backward_motion.vertical = 0;
        } else {
            void (**motion_modes)(MPVContext*) = &context->motion_skipped;

            context->motion_skipped =
                motion_modes[(context->field_344 & 0xC) >> 2];
            if (context->cbp_mask != 0) {
                context->decode_nonintra_blocks(context);
            }
            context->motion_skipped(context);
            context->dc_predictor_y = 0x400;
            context->dc_predictor_cr = 0x400;
            context->dc_predictor_cb = 0x400;
        }

        if (--context->field_1324 <= 0) {
            context->field_1324 = context->condition_state.decoder.field_1AC;
            context->condition_state.decoder.callback(
                context->condition_state.decoder.callback_argument);
        }
        bits = context->bit_reader.bits;
        next_bits = context->bit_reader.next_bits;
        bit_offset = context->bit_reader.bit_offset;
        words = context->bit_reader.words;

        residual_offset = bit_offset & 7;
        consumed = ((const u8*)words +
                    ((bit_offset - residual_offset + 7) >> 3) - 8) -
                   context->header_chunk.data;
        if (context->header_chunk.len - consumed <= 0x800) {
            SJ_SplitChunk(&context->header_chunk, consumed,
                          &context->header_chunk, &refill_remainder);
            stream->interface->put_chunk(
                stream, 0, &context->header_chunk);
            stream->interface->unget_chunk(stream, 1, &refill_remainder);
            stream->interface->get_chunk(
                stream, 1, 0x7FFFFFFF, &context->header_chunk);
            aligned =
                (const u32*)((u32)context->header_chunk.data & ~3);
            byte_offset =
                (context->header_chunk.data - (const u8*)aligned) * 8;
            bits = aligned[0] << byte_offset;
            next_bits = aligned[1];
            words = aligned + 2;
            bit_offset = byte_offset + residual_offset;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= residual_offset;
            }
        }
        first_macroblock = 0;
    }

    consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &final_remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &final_remainder);
    MPV_GoNextDelimSj(stream);
}

static int mpvdec_MotionSub(MPVBitReader* reader, MPVMotionInfo* motion,
                            s32* output, s32* predictor)
{
    u32 bits = reader->bits;
    u32 next_bits = reader->next_bits;
    s32 bit_offset = reader->bit_offset;
    const u32* words = reader->words;
    u32 code = bits >> 21;
    s32 entry;
    s32 motion_code;
    u32 code_length;
    u32 residual;
    s32 delta;
    s32 scaled_code;
    int result = 0;

    if (bit_offset > 21) {
        code |= next_bits >> (53 - bit_offset);
    }
    if ((code >> 7) == 0) {
        entry = mpvvlc_motion_0[code];
    } else {
        entry = mpvvlc_motion_1[code >> 6];
    }

    motion_code = (s8)entry;
    if (motion_code == 0x7F) {
        result = -1;
    } else {
        code_length = (entry >> 8) & 0xFF;
        bit_offset += code_length;
        if (bit_offset >= 32) {
            bit_offset -= 32;
            bits = next_bits << bit_offset;
            next_bits = *words++;
        } else {
            bits <<= code_length;
        }

        if (motion_code == 0) {
            *output = *predictor;
        } else {
            if (motion->r_size != 0) {
                s32 residual_shift = 32 - motion->r_size;
                if (bit_offset >= residual_shift) {
                    bit_offset -= residual_shift;
                    if (bit_offset != 0) {
                        residual =
                            (bits | (next_bits >>
                                     (motion->r_size - bit_offset))) >>
                            residual_shift;
                        bits = next_bits << bit_offset;
                    } else {
                        residual = bits >> residual_shift;
                        bits = next_bits;
                    }
                    next_bits = *words++;
                } else {
                    residual = bits >> residual_shift;
                    bit_offset += motion->r_size;
                    bits <<= motion->r_size;
                }
                scaled_code = (s32)((u32)motion_code << motion->r_size);
                delta = (motion->limit - 1) - residual;
                if (scaled_code > 0) {
                    motion_code = scaled_code - delta;
                } else {
                    motion_code = scaled_code + delta;
                }
            }
            *output = (s32)((u32)(motion_code + *predictor) << motion->shift);
            *output >>= motion->shift;
            *predictor = *output;
        }
        if (motion->full_pel != 0) {
            *output *= 2;
        }
    }

    reader->bits = bits;
    reader->next_bits = next_bits;
    reader->bit_offset = bit_offset;
    reader->words = words;
    return result;
}

void MPVDEC_ResetDc(MPVContext* context)
{
    context->dc_predictor_y = 0x400;
    context->dc_predictor_cr = 0x400;
    context->dc_predictor_cb = 0x400;
}

void MPVDEC_ResetMv(MPVMotionInfo* motion)
{
    motion->previous_horizontal = 0;
    motion->previous_vertical = 0;
    motion->horizontal = 0;
    motion->vertical = 0;
}

void MPVDEC_DecPpicMb(MPVContext* context, SJ* stream)
{
    SJCK refill_remainder;
    SJCK final_remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 peek;
    u32 delta;
    u32 quantizer;
    int bit_offset;
    int byte_offset;
    int residual_offset;
    int consumed;
    int first_macroblock = 1;

    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);
    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    byte_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    bits = aligned[0] << byte_offset;
    next_bits = aligned[1];
    words = aligned + 2;
    bit_offset = byte_offset + context->field_1310;
    if (bit_offset >= 32) {
        bit_offset -= 32;
        bits = next_bits << bit_offset;
        next_bits = *words++;
    } else {
        bits <<= context->field_1310;
    }

    for (;;) {
        int old_index;

        peek = bits >> 9;
        if (bit_offset > 9) {
            peek |= next_bits >> (41 - bit_offset);
        }
        if (peek == 0) {
            break;
        }

        old_index = context->macroblock_index;
        for (;;) {
            int descriptor;
            u8 code_length;
            u8 encoded_increment;
            int increment;

            peek = bits >> 21;
            if (bit_offset > 21) {
                peek |= next_bits >> (53 - bit_offset);
            }
            if ((peek >> 7) == 0) {
                descriptor = mpvvlc_mbai_p_0[peek];
            } else {
                descriptor = mpvvlc_mbai_p_1[peek >> 6];
            }

            code_length = descriptor & 0xF;
            bit_offset += code_length;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }

            encoded_increment = (u32)descriptor >> 2;
            increment = encoded_increment >> 2;
            if (increment == 34) {
                continue;
            }
            if (increment == 35) {
                context->macroblock_index += 33;
                continue;
            }
            if (increment == 36) {
                delta = (u32)-2;
            } else {
                context->macroblock_index += increment;
                context->field_344 = (u32)descriptor >> 10;
                if (context->macroblock_index >
                    context->last_macroblock_index) {
                    delta = (u32)-2;
                } else {
                    delta = context->macroblock_index - old_index;
                    context->macroblock_column += delta;
                    while (context->macroblock_column >=
                           context->condition_state.decoder.picture.
                               macroblocks_per_row) {
                        context->macroblock_column -=
                            context->condition_state.decoder.picture.
                                macroblocks_per_row;
                        context->macroblock_row++;
                    }
                }
            }
            break;
        }

        if (delta == (u32)-2) {
            break;
        }

        if (!first_macroblock && delta > 1) {
            context->skip_macroblocks(context, delta);
            MPVDEC_ResetMv(&context->forward_motion);
            MPVDEC_ResetDc(context);
        }

        if ((context->field_344 & 0x20) == 0) {
            s16 descriptor;
            u8 code_length;

            peek = bits >> 27;
            if (bit_offset > 27) {
                peek |= next_bits >> (59 - bit_offset);
            }
            descriptor = mpvvlc_p_mbtype[peek];
            code_length = descriptor;
            bit_offset += code_length;
            context->field_344 = (u32)descriptor >> 8;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }
        }

        if ((context->field_344 & 0x10) != 0) {
            if (bit_offset >= 27) {
                bit_offset -= 27;
                if (bit_offset != 0) {
                    quantizer =
                        (bits | (next_bits >> (5 - bit_offset))) >> 27;
                    bits = next_bits << bit_offset;
                } else {
                    quantizer = bits >> 27;
                    bits = next_bits;
                }
                next_bits = *words++;
            } else {
                quantizer = bits >> 27;
                bits <<= 5;
                bit_offset += 5;
            }
            context->quantizer_scale = quantizer;
        }

        if ((context->field_344 & 8) != 0) {
            int horizontal_result;
            int vertical_result;

            context->bit_reader.bits = bits;
            context->bit_reader.next_bits = next_bits;
            context->bit_reader.bit_offset = bit_offset;
            context->bit_reader.words = words;
            horizontal_result = mpvdec_MotionSub(
                &context->bit_reader, &context->forward_motion,
                &context->forward_motion.horizontal,
                &context->forward_motion.previous_horizontal);
            vertical_result = mpvdec_MotionSub(
                &context->bit_reader, &context->forward_motion,
                &context->forward_motion.vertical,
                &context->forward_motion.previous_vertical);
            bits = context->bit_reader.bits;
            next_bits = context->bit_reader.next_bits;
            bit_offset = context->bit_reader.bit_offset;
            words = context->bit_reader.words;
            if ((horizontal_result | vertical_result) != 0) {
                break;
            }
        } else {
            MPVDEC_ResetMv(&context->forward_motion);
        }

        if ((context->field_344 & 2) != 0) {
            s16 descriptor;
            u8 code_length;

            peek = bits >> 23;
            if (bit_offset > 23) {
                peek |= next_bits >> (55 - bit_offset);
            }
            descriptor = mpvvlc_cbp[peek];
            code_length = descriptor;
            bit_offset += code_length;
            context->cbp_mask = ((u32)(u16)descriptor << 16) & 0xFFF00000;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }
        } else {
            context->cbp_mask = 0;
        }

        context->bit_reader.bits = bits;
        context->bit_reader.next_bits = next_bits;
        context->bit_reader.bit_offset = bit_offset;
        context->bit_reader.words = words;
        if ((context->field_344 & 1) != 0) {
            context->decode_intra_blocks(context);
            context->motion_intra(context);
        } else {
            if (context->cbp_mask != 0) {
                context->decode_nonintra_blocks(context);
            }
            context->motion_forward(context);
            MPVDEC_ResetDc(context);
        }

        if (--context->field_1324 <= 0) {
            context->field_1324 = context->condition_state.decoder.field_1AC;
            context->condition_state.decoder.callback(
                context->condition_state.decoder.callback_argument);
        }
        bits = context->bit_reader.bits;
        next_bits = context->bit_reader.next_bits;
        bit_offset = context->bit_reader.bit_offset;
        words = context->bit_reader.words;

        residual_offset = bit_offset & 7;
        consumed = ((const u8*)words +
                    ((bit_offset - residual_offset + 7) >> 3) - 8) -
                   context->header_chunk.data;
        if (context->header_chunk.len - consumed <= 0x800) {
            SJ_SplitChunk(&context->header_chunk, consumed,
                          &context->header_chunk, &refill_remainder);
            stream->interface->put_chunk(
                stream, 0, &context->header_chunk);
            stream->interface->unget_chunk(stream, 1, &refill_remainder);
            stream->interface->get_chunk(
                stream, 1, 0x7FFFFFFF, &context->header_chunk);
            aligned =
                (const u32*)((u32)context->header_chunk.data & ~3);
            byte_offset =
                (context->header_chunk.data - (const u8*)aligned) * 8;
            bits = aligned[0] << byte_offset;
            next_bits = aligned[1];
            words = aligned + 2;
            bit_offset = byte_offset + residual_offset;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= residual_offset;
            }
        }
        first_macroblock = 0;
    }

    consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &final_remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &final_remainder);
    MPV_GoNextDelimSj(stream);
}

void MPVDEC_DecIpicMb(MPVContext* context, SJ* stream)
{
    SJCK refill_remainder;
    SJCK final_remainder;
    const u32* aligned;
    const u32* words;
    u32 bits;
    u32 next_bits;
    u32 peek;
    u32 delta;
    u32 quantizer;
    int bit_offset;
    int byte_offset;
    int residual_offset;
    int consumed;

    stream->interface->get_chunk(
        stream, 1, 0x7FFFFFFF, &context->header_chunk);
    aligned = (const u32*)((u32)context->header_chunk.data & ~3);
    byte_offset = (context->header_chunk.data - (const u8*)aligned) * 8;
    next_bits = aligned[1];
    words = aligned + 2;
    bits = aligned[0] << byte_offset;
    bit_offset = byte_offset + context->field_1310;
    if (bit_offset >= 32) {
        bit_offset -= 32;
        bits = next_bits << bit_offset;
        next_bits = *words++;
    } else {
        bits <<= context->field_1310;
    }

    for (;;) {
        int old_index;

        peek = bits >> 9;
        if (bit_offset > 9) {
            peek |= next_bits >> (41 - bit_offset);
        }
        if (peek == 0) {
            break;
        }

        old_index = context->macroblock_index;
        for (;;) {
            int descriptor;
            int code_length;
            u8 encoded_increment;
            int increment;

            peek = bits >> 20;
            if (bit_offset > 20) {
                peek |= next_bits >> (52 - bit_offset);
            }
            if ((peek >> 8) == 0) {
                descriptor = mpvvlc_mbai_i_0[peek];
            } else {
                descriptor = mpvvlc_mbai_i_1[peek >> 6];
            }

            code_length = descriptor & 0xF;
            bit_offset += code_length;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= code_length;
            }

            encoded_increment = descriptor >> 2;
            increment = encoded_increment >> 2;
            if (increment == 34) {
                continue;
            }
            if (increment == 35) {
                context->macroblock_index += 33;
                continue;
            }
            if (increment == 36) {
                delta = (u32)-2;
            } else {
                context->macroblock_index += increment;
                context->field_344 = (u32)descriptor >> 10;
                if (context->macroblock_index >
                    context->last_macroblock_index) {
                    delta = (u32)-2;
                } else {
                    delta = context->macroblock_index - old_index;
                    context->macroblock_column += delta;
                    while (context->macroblock_column >=
                           context->condition_state.decoder.picture.
                               macroblocks_per_row) {
                        context->macroblock_column -=
                            context->condition_state.decoder.picture.
                                macroblocks_per_row;
                        context->macroblock_row++;
                    }
                }
            }
            break;
        }

        if (delta == (u32)-2) {
            break;
        }

        if ((context->field_344 & 0x10) != 0) {
            if (bit_offset >= 27) {
                bit_offset -= 27;
                if (bit_offset != 0) {
                    quantizer =
                        (bits | (next_bits >> (5 - bit_offset))) >> 27;
                    bits = next_bits << bit_offset;
                } else {
                    quantizer = bits >> 27;
                    bits = next_bits;
                }
                next_bits = *words++;
            } else {
                quantizer = bits >> 27;
                bits <<= 5;
                bit_offset += 5;
            }
            context->quantizer_scale = quantizer;
        }

        context->bit_reader.bits = bits;
        context->bit_reader.next_bits = next_bits;
        context->bit_reader.bit_offset = bit_offset;
        context->bit_reader.words = words;
        context->decode_intra_blocks(context);
        context->motion_intra(context);
        if (--context->field_1324 <= 0) {
            context->field_1324 = context->condition_state.decoder.field_1AC;
            context->condition_state.decoder.callback(
                context->condition_state.decoder.callback_argument);
        }
        bits = context->bit_reader.bits;
        next_bits = context->bit_reader.next_bits;
        bit_offset = context->bit_reader.bit_offset;
        words = context->bit_reader.words;

        residual_offset = bit_offset & 7;
        consumed = ((const u8*)words +
                    ((bit_offset - residual_offset + 7) >> 3) - 8) -
                   context->header_chunk.data;
        if (context->header_chunk.len - consumed <= 0x800) {
            SJ_SplitChunk(&context->header_chunk, consumed,
                          &context->header_chunk, &refill_remainder);
            stream->interface->put_chunk(
                stream, 0, &context->header_chunk);
            stream->interface->unget_chunk(stream, 1, &refill_remainder);
            stream->interface->get_chunk(
                stream, 1, 0x7FFFFFFF, &context->header_chunk);
            aligned =
                (const u32*)((u32)context->header_chunk.data & ~3);
            byte_offset =
                (context->header_chunk.data - (const u8*)aligned) * 8;
            next_bits = aligned[1];
            words = aligned + 2;
            bits = aligned[0] << byte_offset;
            bit_offset = byte_offset + residual_offset;
            if (bit_offset >= 32) {
                bit_offset -= 32;
                bits = next_bits << bit_offset;
                next_bits = *words++;
            } else {
                bits <<= residual_offset;
            }
        }
    }

    consumed = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) -
               context->header_chunk.data;
    SJ_SplitChunk(&context->header_chunk, consumed,
                  &context->header_chunk, &final_remainder);
    stream->interface->put_chunk(stream, 0, &context->header_chunk);
    stream->interface->unget_chunk(stream, 1, &final_remainder);
    MPV_GoNextDelimSj(stream);
}

int MPVDEC_CheckVersion(const char* version, u32 object_size, int alignment)
{
    if (strcmp("1.933", version) != 0) {
        return -1;
    }
    if (object_size != 0x1378) {
        return -1;
    }
    return ((alignment - 0x80) | (0x80 - alignment)) >> 31;
}
