#include "sofdec/sfd_transport.h"

/* same shifted-handle view: retail folds the first
 * buffer's offset into each displacement. Direct typed indexing changes
 * SFBUF_VfrmAddRead's codegen, so keep the view tied to SfdHandle's layout. */
typedef struct SfdBufferHn {
    unsigned char pad[0x1308];
    SfdBufferState buffer;
} SfdBufferHn;

static inline int sfbuf_IsMemoryJoint(SJ* sj)
{
    unsigned char buffer[8];
    SJ* probe = SJMEM_Create(buffer, sizeof(buffer));
    const void* uuid = probe->interface->get_uuid(probe);
    int result = sj->interface->get_uuid(sj) == uuid;

    probe->interface->destroy(probe);
    return result;
}

static inline int sfbuf_IsRingJoint(SJ* sj)
{
    unsigned char buffer[8];
    SJ* probe = SJRBF_Create(buffer, sizeof(buffer), 0);
    const void* uuid = probe->interface->get_uuid(probe);
    int result = sj->interface->get_uuid(sj) == uuid;

    probe->interface->destroy(probe);
    return result;
}

long long SFBUF_UpdateFlowCnt(long long count, unsigned int position)
{
    long long wraps;
    long long high;

    if (position < (unsigned int)count) {
        wraps = 1;
    } else {
        wraps = 0;
    }
    high = count >> 32;
    high += wraps;
    high <<= 32;
    high |= position;
    return high;
}

void SFBUF_GetFlowCnt(SJ* sj, int* write_count, int* read_count)
{
    if (sfbuf_IsRingJoint(sj)) {
        *write_count = SJRBF_GetFlowCnt(sj, 1, 1);
        *read_count = SJRBF_GetFlowCnt(sj, 0, 1);
        return;
    }
    if (sfbuf_IsMemoryJoint(sj)) {
        *write_count = SJMEM_GetBufSize(sj);
        *read_count = *write_count - sj->interface->get_num_data(sj, 1);
        return;
    }
    *write_count = 0;
    *read_count = 0;
}

static void sfbuf_RingGetChunks(SJ* sj, int channel, SJCK* first,
                                SJCK* second)
{
    int size;

    size = sj->interface->get_num_data(sj, channel);
    sj->interface->get_chunk(sj, channel, 0x7FFFFFFF, first);
    if (first->len < size) {
        sj->interface->get_chunk(sj, channel, 0x7FFFFFFF, second);
        sj->interface->unget_chunk(sj, channel, second);
    } else {
        second->data = 0;
        second->len = 0;
    }
    sj->interface->unget_chunk(sj, channel, first);
}

static int sfbuf_RingGetDataSizeHn(SfdBufferState* buffer)
{
    SJ* sj;
    SJCK first;
    SJCK second;
    int second_size;
    int first_size;

    first_size = 0;
    second_size = 0;
    sj = buffer->work.ring.supply.stream_joint;
    if (buffer->active != 0 && sj != 0) {
        sfbuf_RingGetChunks(sj, 1, &first, &second);
        first_size = first.len;
        second_size = second.len;
    }
    return first_size + second_size;
}

int SFBUF_RingGetDataSiz(SfdHandle* handle, int buffer_index)
{
    return sfbuf_RingGetDataSizeHn(&handle->buffers[buffer_index]);
}

/* Retail queries inactive transport index 8 as well as buffers 0..7.
 * That address is frame zero's height, not a ninth SfdBufferState. Walk the
 * containing handle representation so the access does not index past buffers.
 */
int SFBUF_GetTermFlg(SfdHandle* handle, int buffer_index)
{
    const unsigned char* object = (const unsigned char*)handle;
    const unsigned char* first_flag =
        (const unsigned char*)&handle->buffers[0].terminated;
    return *(const int*)(object + (first_flag - object) +
                         buffer_index * sizeof(SfdBufferState));
}

void SFBUF_SetTermFlg(SfdHandle* handle, int buffer_index, int terminated)
{
    /* Inactive output index 8 aliases frame zero's height, as in the getter. */
    unsigned char* object = (unsigned char*)handle;
    unsigned char* first_flag = (unsigned char*)&handle->buffers[0].terminated;
    *(int*)(object + (first_flag - object) +
            buffer_index * sizeof(SfdBufferState)) = terminated;
}

/* Inactive output index 8 aliases frame zero's width, like the termination
 * flag aliases its height. Both preparation accesses use the handle extent. */
int SFBUF_GetPrepFlg(SfdHandle* handle, int buffer_index)
{
    const unsigned char* object = (const unsigned char*)handle;
    const unsigned char* first_flag =
        (const unsigned char*)&handle->buffers[0].prepared;
    return *(const int*)(object + (first_flag - object) +
                         buffer_index * sizeof(SfdBufferState));
}

void SFBUF_SetPrepFlg(SfdHandle* handle, int buffer_index, int prepared)
{
    unsigned char* object = (unsigned char*)handle;
    unsigned char* first_flag = (unsigned char*)&handle->buffers[0].prepared;
    *(int*)(object + (first_flag - object) +
            buffer_index * sizeof(SfdBufferState)) = prepared;
}

int SFBUF_VfrmAddRead(SfdHandle* handle, int buffer_index,
                      SfdTransportValue amount)
{
    int result = 0;
    SfdBufferHn* hn = (SfdBufferHn*)((unsigned char*)handle +
                                     buffer_index * sizeof(SfdBufferState));

    if (hn->buffer.active == 0) {
        result = SFTRN_CallTrtTrif(handle, hn->buffer.input_transport, 12,
                                  amount, 0);
    }
    handle->field_0044 = 1;
    return result;
}

int SFBUF_VfrmGetRead(SfdHandle* handle, int buffer_index, void** output)
{
    SfdBufferHn* hn = (SfdBufferHn*)((unsigned char*)handle +
                                     buffer_index * sizeof(SfdBufferState));
    if (hn->buffer.active == 0) {
        return SFTRN_CallTrtTrif(handle, hn->buffer.input_transport, 11,
                                (SfdTransportValue)output, 0);
    }
    return 0;
}

void SFBUF_AddRtotSj(SfdHandle* handle, int buffer_index, int amount)
{
    SfdBufferHn* hn = (SfdBufferHn*)((unsigned char*)handle +
                                     buffer_index * sizeof(SfdBufferState));

    if (hn->buffer.work.ring.read_total >= 0) {
        hn->buffer.work.ring.read_total += amount;
    }
}

int SFBUF_RingGetSj(SfdHandle* handle, int buffer_index, SJ** output)
{
    *output = 0;
    if (handle->buffers[buffer_index].active == 0) {
        return SFLIB_SetErr(handle, 0xFF000401);
    }
    *output = handle->buffers[buffer_index].work.ring.supply.stream_joint;
    return 0;
}

int SFBUF_GetWTot(SfdHandle* handle, int buffer_index)
{
    SfdBufferHn* hn = (SfdBufferHn*)((unsigned char*)handle +
                                     buffer_index * sizeof(SfdBufferState));
    int token;
    int write_total;
    int read_total;

    SFLIB_LockCs(&token);
    write_total = hn->buffer.work.ring.write_total;
    read_total = hn->buffer.work.ring.read_total;
    if (write_total == 0 && read_total != 0) {
        write_total = read_total +
            hn->buffer.work.ring.supply.stream_joint->interface->get_num_data(
                hn->buffer.work.ring.supply.stream_joint, 1);
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
    return handle->buffers[buffer_index].work.ring.supply.buffer_size;
}

void SFBUF_RingSetDlm(SfdHandle* handle, int buffer_index,
                      unsigned char* position, unsigned char* end_position)
{
    SfdBufferHn* hn = (SfdBufferHn*)((unsigned char*)handle +
                                     buffer_index * sizeof(SfdBufferState));
    int token;

    SFLIB_LockCs(&token);
    hn->buffer.work.ring.delimiter_position = position;
    hn->buffer.work.ring.delimiter_end = end_position;
    SFLIB_UnlockCs(&token);
}

void SFBUF_RingGetDlm(SfdHandle* handle, int buffer_index,
                      unsigned char** position, unsigned char** end_position)
{
    SfdBufferHn* hn = (SfdBufferHn*)((unsigned char*)handle +
                                     buffer_index * sizeof(SfdBufferState));
    int token;

    SFLIB_LockCs(&token);
    *position = hn->buffer.work.ring.delimiter_position;
    *end_position = hn->buffer.work.ring.delimiter_end;
    SFLIB_UnlockCs(&token);
}

static inline int sfbuf_RingAddReadSub(SfdHandle* handle, int buffer_index,
                                       int amount)
{
    SJCK first;
    SJCK wrap;
    int result = 0;
    int needed;
    SJCK remainder;
    SJCK chunk;
    SJ* source;
    SJ* sj;
    SfdBufferRingWork* ring;
    SfdBufferState* buffer;
    SfdBufferHn* hn;

    hn = (SfdBufferHn*)((unsigned char*)handle +
                        buffer_index * sizeof(SfdBufferState));
    buffer = &hn->buffer;
    ring = &buffer->work.ring;
    sj = hn->buffer.work.ring.supply.stream_joint;
    if (amount == 0) {
        result = 0;
    } else if (buffer->active == 0 || sj == 0) {
        result = 0;
    } else {
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
            source = ring->supply.stream_joint;
            sfbuf_RingGetChunks(source, 1, &first, &wrap);
            if ((ring->delimiter_position < first.data ||
                 ring->delimiter_position >= first.data + first.len) &&
                (ring->delimiter_position < wrap.data ||
                 ring->delimiter_position >= wrap.data + wrap.len)) {
                ring->delimiter_position = 0;
                ring->delimiter_end = 0;
            }
        }
        if (ring->read_total >= 0) {
            ring->read_total += amount;
        }
        handle->field_0044 = 1;
    }
    return result;
}

int SFBUF_RingAddRead(SfdHandle* handle, int buffer_index, int amount)
{
    return sfbuf_RingAddReadSub(handle, buffer_index, amount);
}

static inline int sfbuf_RingAddWriteSub(SfdHandle* handle, int buffer_index,
                                        int amount)
{
    int needed;
    int result = 0;
    SJCK remainder;
    SJCK chunk;
    SfdBufferRingWork* ring;
    SJ* sj;
    SfdBufferState* buffer;
    SfdBufferHn* hn;

    hn = (SfdBufferHn*)((unsigned char*)handle +
                        buffer_index * sizeof(SfdBufferState));
    buffer = &hn->buffer;
    ring = &buffer->work.ring;
    sj = hn->buffer.work.ring.supply.stream_joint;
    if (amount == 0) {
        result = 0;
    } else if (buffer->active == 0 || sj == 0) {
        result = 0;
    } else {
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
    }
    return result;
}

int SFBUF_RingAddWrite(SfdHandle* handle, int buffer_index, int amount,
                       int value)
{
    return sfbuf_RingAddWriteSub(handle, buffer_index, amount);
}

int SFBUF_RingGetRead(SfdHandle* handle, int buffer_index,
                      SfdBufferTransfer* transfer)
{
    SfdBufferHn* hn;
    SJ* sj;
    SJCK second;
    SJCK first;

    transfer->chunks[0].data = 0;
    transfer->chunks[0].len = 0;
    transfer->chunks[1].data = 0;
    transfer->chunks[1].len = 0;
    transfer->reserved[0] = 0;
    transfer->reserved[1] = 0;
    transfer->reserved[2] = 0;
    hn = (SfdBufferHn*)((unsigned char*)handle +
                        buffer_index * sizeof(SfdBufferState));
    sj = hn->buffer.work.ring.supply.stream_joint;
    if (hn->buffer.active == 0 || sj == 0) {
        return 0;
    }
    sfbuf_RingGetChunks(sj, 1, &first, &second);
    transfer->chunks[0].data = first.data;
    transfer->chunks[0].len = first.len;
    transfer->chunks[1].data = second.data;
    transfer->chunks[1].len = second.len;
    return 0;
}

int SFBUF_RingGetWrite(SfdHandle* handle, int buffer_index,
                       SfdBufferTransfer* transfer)
{
    SfdBufferHn* hn;
    SJ* sj;
    SJCK second;
    SJCK first;

    transfer->chunks[0].data = 0;
    transfer->chunks[0].len = 0;
    transfer->chunks[1].data = 0;
    transfer->chunks[1].len = 0;
    transfer->reserved[0] = 0;
    transfer->reserved[1] = 0;
    transfer->reserved[2] = 0;
    hn = (SfdBufferHn*)((unsigned char*)handle +
                        buffer_index * sizeof(SfdBufferState));
    sj = hn->buffer.work.ring.supply.stream_joint;
    if (hn->buffer.active == 0 || sj == 0) {
        return 0;
    }
    sfbuf_RingGetChunks(sj, 0, &first, &second);
    transfer->chunks[0].data = first.data;
    transfer->chunks[0].len = first.len;
    transfer->chunks[1].data = second.data;
    transfer->chunks[1].len = second.len;
    return 0;
}

void SFBUF_GetUoch(SfdHandle* handle, int buffer_index, int channel,
                   SfdBufferChannel* output)
{
    *output = handle->buffers[buffer_index].work.user_channels[channel];
}

void SFBUF_SetUoch(SfdHandle* handle, int buffer_index, int channel,
                   SfdBufferChannel* input)
{
    handle->buffers[buffer_index].work.user_channels[channel] = *input;
}

static int sfbuf_CheckSupply(SfdBufferSupply* supply)
{
    if (supply->stream_joint == 0) {
        return -1;
    }
    if (supply->kind == 0) {
        if (supply->buffer == 0) {
            return -1;
        }
        if (supply->buffer_size <= 0) {
            return -1;
        }
        if (supply->field_14 > 0) {
            return -1;
        }
    }
    return 0;
}

static void sfbuf_SetSupply(SfdBufferState* buffer, SfdBufferRingWork* ring,
                            SfdBufferSupply* supply, int active)
{
    int token;

    SFLIB_LockCs(&token);
    buffer->active = active;
    ring->supply = *supply;
    ring->delimiter_position = 0;
    ring->delimiter_end = 0;
    ring->write_total = 0;
    ring->read_total = 0;
    SFPTS_InitPtsQue(&ring->pts_queue);
    SFLIB_UnlockCs(&token);
}

int SFBUF_SetSupplySj(SfdHandle* handle, SfdBufferSupply* supply)
{
    int buffer_index;
    SfdBufferRingWork* ring;
    SfdBufferState* buffer;
    SfdBufferHn* hn;

    if (sfbuf_CheckSupply(supply) != 0) {
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
    hn = (SfdBufferHn*)((unsigned char*)handle +
                        buffer_index * sizeof(SfdBufferState));
    buffer = &hn->buffer;
    ring = &buffer->work.ring;
    if (hn->buffer.storage_mode != 4) {
        return SFLIB_SetErr(handle, 0xFF000409);
    }
    sfbuf_SetSupply(buffer, ring, supply, supply->stream_joint != 0);
    return 0;
}

static void sfbuf_DestroySupply(SfdBufferSupply* supply)
{
    if (supply->stream_joint != 0) {
        supply->stream_joint->interface->destroy(supply->stream_joint);
        supply->stream_joint = 0;
    }
}

void SFBUF_DestroySj(SfdHandle* handle)
{
    SfdBufferState* buffer;
    SfdBufferSupply* supply;

    buffer = &handle->buffers[0];
    supply = &buffer->work.ring.supply;
    if (buffer->storage_mode == 5) {
        sfbuf_DestroySupply(supply);
    }
    buffer = &handle->buffers[1];
    supply = &buffer->work.ring.supply;
    if (buffer->storage_mode == 5) {
        sfbuf_DestroySupply(supply);
    }
    buffer = &handle->buffers[2];
    supply = &buffer->work.ring.supply;
    if (buffer->storage_mode == 5) {
        sfbuf_DestroySupply(supply);
    }
}

static int sfbuf_CreateStreamJoint(SfdBufferSupply* supply,
                                   unsigned int address, int buffer_size,
                                   int extra_size)
{
    if (buffer_size <= 0) {
        return SFLIB_SetErr(0, 0xFF00040C);
    }
    supply->extra_size = extra_size;
    supply->field_14 = 0;
    supply->stream_joint = SJRBF_Create((unsigned char*)address, buffer_size,
                                        extra_size);
    if (supply->stream_joint == 0) {
        return SFLIB_SetErr(0, 0xFF00040A);
    }
    return 0;
}

static inline int sfbuf_InitRing(SfdBufferState* buffer,
                                 unsigned int* address, int size,
                                 int extra_size)
{
    int active;
    int storage_mode;
    SfdBufferSupply supply;
    int result;

    if (size == 0) {
        active = 0;
        storage_mode = 4;
    } else {
        active = 1;
        storage_mode = 5;
        supply.kind = 0;
        supply.buffer = (unsigned char*)*address;
        supply.buffer_size = size - extra_size;
        result = sfbuf_CreateStreamJoint(&supply, *address,
                                         supply.buffer_size, extra_size);
        if (result != 0) {
            return result;
        }
        sfbuf_SetSupply(buffer, &buffer->work.ring, &supply, 1);
    }
    buffer->storage_mode = storage_mode;
    buffer->active = active;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    return 0;
}

static inline void sfbuf_InitVideo(SfdHandle* handle, SfdBufferState* buffer,
                                   unsigned int* address, int* size)
{
    long index; /* CRI Sint32 (signed long) frame counter */
    int active;

    active = *size != 0;
    buffer->storage_mode = 1;
    buffer->active = active;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.video.buffer = (unsigned char*)*address;
    buffer->work.video.buffer_size = *size;
    buffer->work.video.reserved_08 = 0;
    buffer->work.video.reserved_0C = 0;
    buffer->work.video.video_frames = handle->video_frames;
    for (index = 0; index < 16; index++) {
        buffer->work.video.video_frames[index].state = 0;
    }
}

static inline void sfbuf_InitAudio(SfdBufferState* buffer,
                                   unsigned int* address, int* size)
{
    int index;
    int active;

    active = *size != 0;
    buffer->storage_mode = 2;
    buffer->active = active;
    buffer->prepared = 0;
    buffer->terminated = 0;
    buffer->input_transport = 9;
    buffer->output_transport = 9;
    buffer->work.audio.buffer = (unsigned char*)*address;
    buffer->work.audio.buffer_size = *size;
    for (index = 0; index < 7; index++) {
        buffer->work.audio.reserved[index] = 0;
    }
    buffer->work.audio.reserved_tail[0] = 0;
    buffer->work.audio.reserved_tail[1] = 0;
    buffer->work.audio.reserved_tail[2] = 0;
}

int SFBUF_InitHn(SfdHandle* handle, SfdBufferState* buffers,
                 SfdBufferCreateConfig* create)
{
    unsigned int addresses[9];
    int remainder;
    int result;
    int index;
    unsigned int* cursor;
    unsigned int current;

    current = (unsigned int)create->memory;
    cursor = addresses;
    for (index = 0; index < 7; index++) {
        *cursor++ = current;
        current += create->buffer_sizes[index];
    }
    *cursor = current;

    remainder = create->buffer_sizes[0] % create->ring_alignment;
    result = sfbuf_InitRing(&buffers[0], &addresses[0],
                            create->buffer_sizes[0], remainder);
    if (result != 0) {
        return result;
    }

    result = sfbuf_InitRing(&buffers[1], &addresses[1],
                            create->buffer_sizes[1], 0x800);
    if (result != 0) {
        return result;
    }

    result = sfbuf_InitRing(&buffers[2], &addresses[2],
                            create->buffer_sizes[2], 0);
    if (result != 0) {
        return result;
    }

    sfbuf_InitVideo(handle, &buffers[3], &addresses[3],
                    &create->buffer_sizes[3]);

    sfbuf_InitAudio(&buffers[4], &addresses[4],
                    &create->buffer_sizes[4]);

    sfbuf_InitVideo(handle, &buffers[5], &addresses[5],
                    &create->buffer_sizes[5]);

    sfbuf_InitAudio(&buffers[6], &addresses[6],
                    &create->buffer_sizes[6]);

    buffers[7].storage_mode = 3;
    buffers[7].active = 1;
    buffers[7].prepared = 0;
    buffers[7].terminated = 0;
    buffers[7].input_transport = 9;
    buffers[7].output_transport = 9;
    for (index = 0; index < 3; index++) {
        buffers[7].work.user_channels[index].stream_joint = 0;
        buffers[7].work.user_channels[index].object = 0;
        buffers[7].work.user_channels[index].handle_callback = 0;
        buffers[7].work.user_channels[index].object_callback = 0;
    }
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
