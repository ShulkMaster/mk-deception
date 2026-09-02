#include "sofdec/sfd_transport.h"

typedef struct SfdBufferInitWork {
    int tokens[3];
    SfdBufferSupply supplies[3];
    unsigned char* memory[8];
} SfdBufferInitWork;

typedef char SfdBufferInitWorkSizeCheck[
    sizeof(SfdBufferInitWork) == 0x74 ? 1 : -1];

long long SFBUF_UpdateFlowCnt(int count, unsigned int old_position,
                              unsigned int position)
{
    long long wraps;

    if (position < old_position) {
        wraps = 1;
    } else {
        wraps = 0;
    }
    return position | (((long long)count + wraps) << 32);
}

void SFBUF_GetFlowCnt(SJ* sj, int* write_count, int* read_count)
{
    unsigned char rbf_buffer[8];
    unsigned char mem_buffer[8];
    SJ* probe;
    const void* uuid;

    probe = SJRBF_Create(rbf_buffer, sizeof(rbf_buffer), 0);
    uuid = probe->interface->get_uuid(probe);
    if (uuid == sj->interface->get_uuid(sj)) {
        probe->interface->destroy(probe);
        *write_count = SJRBF_GetFlowCnt(sj, 1, 1);
        *read_count = SJRBF_GetFlowCnt(sj, 0, 1);
        return;
    }
    probe->interface->destroy(probe);
    probe = SJMEM_Create(mem_buffer, sizeof(mem_buffer));
    uuid = probe->interface->get_uuid(probe);
    if (uuid == sj->interface->get_uuid(sj)) {
        probe->interface->destroy(probe);
        *write_count = SJMEM_GetBufSize(sj);
        *read_count = *write_count - sj->interface->get_num_data(sj, 1);
        return;
    }
    probe->interface->destroy(probe);
    *write_count = 0;
    *read_count = 0;
}

int SFBUF_RingGetDataSiz(SfdHandle* handle, int buffer_index)
{
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    SJ* sj = buffer->work.ring.stream_joint;
    SJCK first;
    SJCK second;
    int size = 0;

    if (buffer->active != 0 && sj != 0) {
        size = sj->interface->get_num_data(sj, 1);
        sj->interface->get_chunk(sj, 1, 0x7FFFFFFF, &first);
        if (first.len < size) {
            sj->interface->get_chunk(sj, 1, 0x7FFFFFFF, &second);
            sj->interface->unget_chunk(sj, 1, &second);
        } else {
            second.data = 0;
            second.len = 0;
        }
        sj->interface->unget_chunk(sj, 1, &first);
        return first.len + second.len;
    }
    return 0;
}

int SFBUF_GetTermFlg(SfdHandle* handle, int buffer_index)
{
    return handle->buffers[buffer_index].terminated;
}

void SFBUF_SetTermFlg(SfdHandle* handle, int buffer_index, int terminated)
{
    handle->buffers[buffer_index].terminated = terminated;
}

int SFBUF_GetPrepFlg(SfdHandle* handle, int buffer_index)
{
    return handle->buffers[buffer_index].prepared;
}

void SFBUF_SetPrepFlg(SfdHandle* handle, int buffer_index, int prepared)
{
    handle->buffers[buffer_index].prepared = prepared;
}

int SFBUF_VfrmAddRead(SfdHandle* handle, int buffer_index,
                      SfdTransportValue amount)
{
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    int result = 0;
    if (buffer->active == 0) {
        result = SFTRN_CallTrtTrif(handle, buffer->input_transport, 12,
                                  amount, 0);
    }
    handle->field_0044 = 1;
    return result;
}

int SFBUF_VfrmGetRead(SfdHandle* handle, int buffer_index, void* output)
{
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    if (buffer->active == 0) {
        return SFTRN_CallTrtTrif(handle, buffer->input_transport, 11,
                                (SfdTransportValue)output, 0);
    }
    return 0;
}

void SFBUF_AddRtotSj(SfdHandle* handle, int buffer_index, int amount)
{
    int* total = &handle->buffers[buffer_index].work.ring.read_total;
    if (*total >= 0) {
        *total += amount;
    }
}

int SFBUF_RingGetSj(SfdHandle* handle, int buffer_index, SJ** output)
{
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    *output = 0;
    if (buffer->active == 0) {
        return SFLIB_SetErr(handle, 0xFF000401);
    }
    *output = buffer->work.ring.stream_joint;
    return 0;
}

int SFBUF_GetWTot(SfdHandle* handle, int buffer_index)
{
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    int token;
    int write_total;
    int read_total;
    SFLIB_LockCs(&token);
    write_total = ring->write_total;
    read_total = ring->read_total;
    if (write_total == 0 && read_total != 0) {
        write_total = read_total +
            ring->stream_joint->interface->get_num_data(ring->stream_joint, 1);
    }
    if (write_total < 0) {
        write_total = 0x7FFFFFFF;
    }
    SFLIB_UnlockCs(&token);
    return write_total;
}

int SFBUF_GetRTot(SfdHandle* handle, int buffer_index)
{
    return handle->buffers[buffer_index].work.ring.read_total;
}

int SFBUF_GetRingBufSiz(SfdHandle* handle, int buffer_index)
{
    return handle->buffers[buffer_index].work.ring.buffer_size;
}

void SFBUF_RingSetDlm(SfdHandle* handle, int buffer_index,
                      unsigned char* position, unsigned char* end_position)
{
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    int token;
    SFLIB_LockCs(&token);
    ring->delimiter_position = position;
    ring->delimiter_end = end_position;
    SFLIB_UnlockCs(&token);
}

void SFBUF_RingGetDlm(SfdHandle* handle, int buffer_index,
                      unsigned char** position, unsigned char** end_position)
{
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    int token;
    SFLIB_LockCs(&token);
    *position = ring->delimiter_position;
    *end_position = ring->delimiter_end;
    SFLIB_UnlockCs(&token);
}

int SFBUF_RingAddRead(SfdHandle* handle, int buffer_index, int amount)
{
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    SfdBufferRingWork* ring = &buffer->work.ring;
    SJ* sj = ring->stream_joint;
    SJCK chunk;
    SJCK remainder;
    int result = 0;
    int needed;

    if (amount == 0 || buffer->active == 0 || sj == 0) {
        return 0;
    }
    sj->interface->get_chunk(sj, 1, amount, &chunk);
    sj->interface->put_chunk(sj, 0, &chunk);
    if (chunk.len < amount) {
        needed = amount - chunk.len;
        sj->interface->get_chunk(sj, 1, needed, &remainder);
        sj->interface->put_chunk(sj, 0, &remainder);
        if (remainder.len < needed) {
            result = SFLIB_SetErr(handle, 0xFF00040B);
        }
    }
    if (buffer_index == 1) {
        SJ* source = ring->stream_joint;
        int size = source->interface->get_num_data(source, 1);
        SJCK first;
        SJCK second;
        source->interface->get_chunk(source, 1, 0x7FFFFFFF, &first);
        if (first.len < size) {
            source->interface->get_chunk(source, 1, 0x7FFFFFFF, &second);
            source->interface->unget_chunk(source, 1, &second);
        } else {
            second.data = 0;
            second.len = 0;
        }
        source->interface->unget_chunk(source, 1, &first);
        if (!((ring->delimiter_position >= first.data &&
               ring->delimiter_position < first.data + first.len) ||
              (ring->delimiter_position >= second.data &&
               ring->delimiter_position < second.data + second.len))) {
            ring->delimiter_position = 0;
            ring->delimiter_end = 0;
        }
    }
    if (ring->read_total >= 0) {
        ring->read_total += amount;
    }
    handle->field_0044 = 1;
    return result;
}

int SFBUF_RingAddWrite(SfdHandle* handle, int buffer_index, int amount,
                       int value)
{
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    SfdBufferRingWork* ring = &buffer->work.ring;
    SJ* sj = ring->stream_joint;
    SJCK chunk;
    SJCK remainder;
    int result = 0;
    int needed;

    if (amount == 0 || buffer->active == 0 || sj == 0) {
        return 0;
    }
    sj->interface->get_chunk(sj, 0, amount, &chunk);
    sj->interface->put_chunk(sj, 1, &chunk);
    if (chunk.len < amount) {
        needed = amount - chunk.len;
        sj->interface->get_chunk(sj, 0, needed, &remainder);
        sj->interface->put_chunk(sj, 1, &remainder);
        if (remainder.len < needed) {
            result = SFLIB_SetErr(handle, 0xFF00040B);
        }
    }
    if (ring->write_total >= 0) {
        ring->write_total += amount;
    }
    handle->field_0044 = 1;
    return result;
}

int SFBUF_RingGetRead(SfdHandle* handle, int buffer_index, void* output)
{
    SfdBufferTransfer* transfer = output;
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    SJ* sj = buffer->work.ring.stream_joint;
    int size;

    transfer->chunks[0].data = 0;
    transfer->chunks[0].len = 0;
    transfer->chunks[1].data = 0;
    transfer->chunks[1].len = 0;
    transfer->field_10 = 0;
    transfer->field_14 = 0;
    transfer->field_18 = 0;
    if (buffer->active == 0 || sj == 0) {
        return 0;
    }
    size = sj->interface->get_num_data(sj, 1);
    sj->interface->get_chunk(sj, 1, 0x7FFFFFFF, &transfer->chunks[0]);
    if (transfer->chunks[0].len < size) {
        sj->interface->get_chunk(sj, 1, 0x7FFFFFFF, &transfer->chunks[1]);
        sj->interface->unget_chunk(sj, 1, &transfer->chunks[1]);
    }
    sj->interface->unget_chunk(sj, 1, &transfer->chunks[0]);
    return 0;
}

int SFBUF_RingGetWrite(SfdHandle* handle, int buffer_index, void* output)
{
    SfdBufferTransfer* transfer = output;
    SfdBufferState* buffer = &handle->buffers[buffer_index];
    SJ* sj = buffer->work.ring.stream_joint;
    int size;

    transfer->chunks[0].data = 0;
    transfer->chunks[0].len = 0;
    transfer->chunks[1].data = 0;
    transfer->chunks[1].len = 0;
    transfer->field_10 = 0;
    transfer->field_14 = 0;
    transfer->field_18 = 0;
    if (buffer->active == 0 || sj == 0) {
        return 0;
    }
    size = sj->interface->get_num_data(sj, 0);
    sj->interface->get_chunk(sj, 0, 0x7FFFFFFF, &transfer->chunks[0]);
    if (transfer->chunks[0].len < size) {
        sj->interface->get_chunk(sj, 0, 0x7FFFFFFF, &transfer->chunks[1]);
        sj->interface->unget_chunk(sj, 0, &transfer->chunks[1]);
    }
    sj->interface->unget_chunk(sj, 0, &transfer->chunks[0]);
    return 0;
}

void SFBUF_GetUoch(SfdHandle* handle, int buffer_index, int channel,
                   SfdBufferChannel* output)
{
    *output = handle->buffers[buffer_index].work.user_channels[channel];
}

void SFBUF_SetUoch(SfdHandle* handle, int buffer_index, int channel,
                   const SfdBufferChannel* input)
{
    handle->buffers[buffer_index].work.user_channels[channel] = *input;
}

int SFBUF_SetSupplySj(SfdHandle* handle, const SfdBufferSupply* supply)
{
    SfdBufferState* buffer;
    SfdBufferRingWork* ring;
    int buffer_index;
    int token;

    if (supply->stream_joint == 0 ||
        (supply->field_00 == 0 &&
         (supply->buffer == 0 || supply->buffer_size <= 0 ||
          supply->field_14 > 0))) {
        return SFLIB_SetErr(handle, 0xFF000408);
    }
    if (SFTRN_IsSetup(handle, 1) != 0) {
        buffer_index = 0;
    } else if (SFTRN_IsSetup(handle, 2) != 0) {
        buffer_index = 1;
    } else if (SFTRN_IsSetup(handle, 3) != 0) {
        buffer_index = 2;
    } else {
        buffer_index = 0;
    }
    buffer = &handle->buffers[buffer_index];
    ring = &buffer->work.ring;
    if (buffer->storage_mode != 4) {
        return SFLIB_SetErr(handle, 0xFF000409);
    }
    SFLIB_LockCs(&token);
    buffer->active = supply->stream_joint != 0;
    ring->field_00 = supply->field_00;
    ring->stream_joint = supply->stream_joint;
    ring->buffer = supply->buffer;
    ring->buffer_size = supply->buffer_size;
    ring->field_10 = supply->field_10;
    ring->field_14 = supply->field_14;
    ring->delimiter_position = 0;
    ring->delimiter_end = 0;
    ring->write_total = 0;
    ring->read_total = 0;
    SFPTS_InitPtsQue(&ring->pts_queue);
    SFLIB_UnlockCs(&token);
    return 0;
}

void SFBUF_DestroySj(SfdHandle* handle)
{
    SfdBufferState* buffer;

    buffer = &handle->buffers[0];
    if (buffer->storage_mode == 5 && buffer->work.ring.stream_joint != 0) {
        buffer->work.ring.stream_joint->interface->destroy(
            buffer->work.ring.stream_joint);
        buffer->work.ring.stream_joint = 0;
    }
    buffer = &handle->buffers[1];
    if (buffer->storage_mode == 5 && buffer->work.ring.stream_joint != 0) {
        buffer->work.ring.stream_joint->interface->destroy(
            buffer->work.ring.stream_joint);
        buffer->work.ring.stream_joint = 0;
    }
    buffer = &handle->buffers[2];
    if (buffer->storage_mode == 5 && buffer->work.ring.stream_joint != 0) {
        buffer->work.ring.stream_joint->interface->destroy(
            buffer->work.ring.stream_joint);
        buffer->work.ring.stream_joint = 0;
    }
}

int SFBUF_InitHn(SfdHandle* handle, SfdBufferState* buffers,
                 const SfdBufferCreateConfig* create)
{
    SfdBufferInitWork init;
    SfdBufferState* buffer;
    int active;
    int storage_mode;
    int remainder;
    int result;
    int index;

    init.memory[0] = create->memory;
    for (index = 0; index < 7; index++) {
        init.memory[index + 1] =
            init.memory[index] + create->buffer_sizes[index];
    }

    remainder = create->buffer_sizes[0] % create->ring_alignment;
    buffer = &buffers[0];
    if (create->buffer_sizes[0] == 0) {
        active = 0;
        storage_mode = 4;
    } else {
        active = 1;
        storage_mode = 5;
        init.supplies[2].field_00 = 0;
        init.supplies[2].buffer = init.memory[0];
        init.supplies[2].buffer_size = create->buffer_sizes[0] - remainder;
        if (init.supplies[2].buffer_size <= 0) {
            result = SFLIB_SetErr(0, 0xFF00040C);
        } else {
            init.supplies[2].field_10 = remainder;
            init.supplies[2].field_14 = 0;
            init.supplies[2].stream_joint = SJRBF_Create(
                init.supplies[2].buffer, init.supplies[2].buffer_size,
                init.supplies[2].field_10);
            result = init.supplies[2].stream_joint == 0 ?
                SFLIB_SetErr(0, 0xFF00040A) : 0;
        }
        if (result != 0) {
            return result;
        }
        SFLIB_LockCs(&init.tokens[2]);
        buffer->active = 1;
        buffer->work.ring.field_00 = init.supplies[2].field_00;
        buffer->work.ring.stream_joint = init.supplies[2].stream_joint;
        buffer->work.ring.buffer = init.supplies[2].buffer;
        buffer->work.ring.buffer_size = init.supplies[2].buffer_size;
        buffer->work.ring.field_10 = init.supplies[2].field_10;
        buffer->work.ring.field_14 = init.supplies[2].field_14;
        buffer->work.ring.delimiter_position = 0;
        buffer->work.ring.delimiter_end = 0;
        buffer->work.ring.write_total = 0;
        buffer->work.ring.read_total = 0;
        SFPTS_InitPtsQue(&buffer->work.ring.pts_queue);
        SFLIB_UnlockCs(&init.tokens[2]);
    }
    buffer->storage_mode = storage_mode;
    buffer->active = active;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;

    buffer = &buffers[1];
    if (create->buffer_sizes[1] == 0) {
        active = 0;
        storage_mode = 4;
    } else {
        active = 1;
        storage_mode = 5;
        init.supplies[1].field_00 = 0;
        init.supplies[1].buffer = init.memory[1];
        init.supplies[1].buffer_size = create->buffer_sizes[1] - 0x800;
        if (init.supplies[1].buffer_size <= 0) {
            result = SFLIB_SetErr(0, 0xFF00040C);
        } else {
            init.supplies[1].field_10 = 0x800;
            init.supplies[1].field_14 = 0;
            init.supplies[1].stream_joint = SJRBF_Create(
                init.supplies[1].buffer, init.supplies[1].buffer_size,
                init.supplies[1].field_10);
            result = init.supplies[1].stream_joint == 0 ?
                SFLIB_SetErr(0, 0xFF00040A) : 0;
        }
        if (result != 0) {
            return result;
        }
        SFLIB_LockCs(&init.tokens[1]);
        buffer->active = 1;
        buffer->work.ring.field_00 = init.supplies[1].field_00;
        buffer->work.ring.stream_joint = init.supplies[1].stream_joint;
        buffer->work.ring.buffer = init.supplies[1].buffer;
        buffer->work.ring.buffer_size = init.supplies[1].buffer_size;
        buffer->work.ring.field_10 = init.supplies[1].field_10;
        buffer->work.ring.field_14 = init.supplies[1].field_14;
        buffer->work.ring.delimiter_position = 0;
        buffer->work.ring.delimiter_end = 0;
        buffer->work.ring.write_total = 0;
        buffer->work.ring.read_total = 0;
        SFPTS_InitPtsQue(&buffer->work.ring.pts_queue);
        SFLIB_UnlockCs(&init.tokens[1]);
    }
    buffer->storage_mode = storage_mode;
    buffer->active = active;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;

    buffer = &buffers[2];
    if (create->buffer_sizes[2] == 0) {
        active = 0;
        storage_mode = 4;
    } else {
        active = 1;
        storage_mode = 5;
        init.supplies[0].field_00 = 0;
        init.supplies[0].buffer = init.memory[2];
        init.supplies[0].buffer_size = create->buffer_sizes[2];
        if (init.supplies[0].buffer_size <= 0) {
            result = SFLIB_SetErr(0, 0xFF00040C);
        } else {
            init.supplies[0].field_10 = 0;
            init.supplies[0].field_14 = 0;
            init.supplies[0].stream_joint = SJRBF_Create(
                init.supplies[0].buffer, init.supplies[0].buffer_size,
                init.supplies[0].field_10);
            result = init.supplies[0].stream_joint == 0 ?
                SFLIB_SetErr(0, 0xFF00040A) : 0;
        }
        if (result != 0) {
            return result;
        }
        SFLIB_LockCs(&init.tokens[0]);
        buffer->active = 1;
        buffer->work.ring.field_00 = init.supplies[0].field_00;
        buffer->work.ring.stream_joint = init.supplies[0].stream_joint;
        buffer->work.ring.buffer = init.supplies[0].buffer;
        buffer->work.ring.buffer_size = init.supplies[0].buffer_size;
        buffer->work.ring.field_10 = init.supplies[0].field_10;
        buffer->work.ring.field_14 = init.supplies[0].field_14;
        buffer->work.ring.delimiter_position = 0;
        buffer->work.ring.delimiter_end = 0;
        buffer->work.ring.write_total = 0;
        buffer->work.ring.read_total = 0;
        SFPTS_InitPtsQue(&buffer->work.ring.pts_queue);
        SFLIB_UnlockCs(&init.tokens[0]);
    }
    buffer->storage_mode = storage_mode;
    buffer->active = active;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;

    buffer = &buffers[3];
    buffer->storage_mode = 1;
    buffer->active = create->buffer_sizes[3] != 0;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.linear.buffer = init.memory[3];
    buffer->work.linear.buffer_size = create->buffer_sizes[3];
    buffer->work.linear.field_08 = 0;
    buffer->work.linear.field_0C = 0;
    buffer->work.linear.video_frames = handle->video_frames;
    handle->video_frames[0].state = 0;
    handle->video_frames[1].state = 0;
    handle->video_frames[2].state = 0;
    handle->video_frames[3].state = 0;
    handle->video_frames[4].state = 0;
    handle->video_frames[5].state = 0;
    handle->video_frames[6].state = 0;
    handle->video_frames[7].state = 0;
    handle->video_frames[8].state = 0;
    handle->video_frames[9].state = 0;
    handle->video_frames[10].state = 0;
    handle->video_frames[11].state = 0;
    handle->video_frames[12].state = 0;
    handle->video_frames[13].state = 0;
    handle->video_frames[14].state = 0;
    handle->video_frames[15].state = 0;

    buffer = &buffers[4];
    buffer->storage_mode = 2;
    buffer->active = create->buffer_sizes[4] != 0;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.linear.buffer = init.memory[4];
    buffer->work.linear.buffer_size = create->buffer_sizes[4];
    buffer->work.linear.field_08 = 0;
    buffer->work.linear.field_0C = 0;
    buffer->work.linear.video_frames = 0;
    buffer->work.linear.field_14 = 0;
    buffer->work.linear.field_18 = 0;
    buffer->work.linear.field_1C = 0;
    buffer->work.linear.field_20 = 0;
    buffer->work.linear.field_24 = 0;
    buffer->work.linear.field_28 = 0;
    buffer->work.linear.field_2C = 0;
    buffer->work.linear.field_30 = 0;
    buffer->work.linear.field_34 = 0;
    buffer->work.linear.field_38 = 0;

    buffer = &buffers[5];
    buffer->storage_mode = 1;
    buffer->active = create->buffer_sizes[5] != 0;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.linear.buffer = init.memory[5];
    buffer->work.linear.buffer_size = create->buffer_sizes[5];
    buffer->work.linear.field_08 = 0;
    buffer->work.linear.field_0C = 0;
    buffer->work.linear.video_frames = handle->video_frames;
    handle->video_frames[0].state = 0;
    handle->video_frames[1].state = 0;
    handle->video_frames[2].state = 0;
    handle->video_frames[3].state = 0;
    handle->video_frames[4].state = 0;
    handle->video_frames[5].state = 0;
    handle->video_frames[6].state = 0;
    handle->video_frames[7].state = 0;
    handle->video_frames[8].state = 0;
    handle->video_frames[9].state = 0;
    handle->video_frames[10].state = 0;
    handle->video_frames[11].state = 0;
    handle->video_frames[12].state = 0;
    handle->video_frames[13].state = 0;
    handle->video_frames[14].state = 0;
    handle->video_frames[15].state = 0;

    buffer = &buffers[6];
    buffer->storage_mode = 2;
    buffer->active = create->buffer_sizes[6] != 0;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.linear.buffer = init.memory[6];
    buffer->work.linear.buffer_size = create->buffer_sizes[6];
    buffer->work.linear.field_08 = 0;
    buffer->work.linear.field_0C = 0;
    buffer->work.linear.video_frames = 0;
    buffer->work.linear.field_14 = 0;
    buffer->work.linear.field_18 = 0;
    buffer->work.linear.field_1C = 0;
    buffer->work.linear.field_20 = 0;
    buffer->work.linear.field_24 = 0;
    buffer->work.linear.field_28 = 0;
    buffer->work.linear.field_2C = 0;
    buffer->work.linear.field_30 = 0;
    buffer->work.linear.field_34 = 0;
    buffer->work.linear.field_38 = 0;

    buffer = &buffers[7];
    buffer->storage_mode = 3;
    buffer->active = 1;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.linear.buffer = 0;
    buffer->work.linear.buffer_size = 0;
    buffer->work.linear.field_08 = 0;
    buffer->work.linear.field_0C = 0;
    buffer->work.linear.video_frames = 0;
    buffer->work.linear.field_14 = 0;
    buffer->work.linear.field_18 = 0;
    buffer->work.linear.field_1C = 0;
    buffer->work.linear.field_20 = 0;
    buffer->work.linear.field_24 = 0;
    buffer->work.linear.field_28 = 0;
    buffer->work.linear.field_2C = 0;
    buffer->work.linear.field_30 = 0;
    buffer->work.linear.field_34 = 0;
    buffer->work.linear.field_38 = 0;
    return 0;
}

void SFBUF_Finish(int* work)
{
    (void)work;
}

void SFBUF_Init(int* work)
{
    (void)work;
}
