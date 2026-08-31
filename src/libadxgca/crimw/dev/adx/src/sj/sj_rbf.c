#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct SJRingBufferUuid {
    u32 time_low;
    u16 time_mid;
    u16 time_high_and_version;
    u8 clock_sequence_and_node[8];
} SJRingBufferUuid;

typedef struct SJRingBuffer {
    const SJInterface* interface;
    int used;
    const SJRingBufferUuid* uuid;
    int data_size;
    int free_size;
    int write_position;
    int read_position;
    u8* buffer;
    int buffer_size;
    int extra_size;
    int flow_count[2][2];
    SJErrorCallback error_callback;
    void* error_object;
} SJRingBuffer;

typedef char SJRingBufferUuidSizeCheck[
    sizeof(SJRingBufferUuid) == 0x10 ? 1 : -1];
typedef char SJRingBufferSizeCheck[
    sizeof(SJRingBuffer) == 0x40 ? 1 : -1];

extern void SJCRS_Lock(void);
extern void SJCRS_Unlock(void);
extern void SJERR_CallErr(const char* message);

int SJRBF_GetXtrSize(SJ* sj);
int SJRBF_GetBufSize(SJ* sj);
u8* SJRBF_GetBufPtr(SJ* sj);
int SJRBF_IsGetChunk(
    SJ* sj, int channel, int size, int* available);
void SJRBF_UngetChunk(SJ* sj, int channel, SJCK* chunk);
void SJRBF_PutChunk(SJ* sj, int channel, SJCK* chunk);
void SJRBF_GetChunk(SJ* sj, int channel, int max_size, SJCK* chunk);
int SJRBF_GetNumData(SJ* sj, int channel);
void SJRBF_Reset(SJ* sj);
void SJRBF_EntryErrFunc(
    SJ* sj, SJErrorCallback callback, void* object);
const void* SJRBF_GetUuid(SJ* sj);
void SJRBF_Destroy(SJ* sj);
void SJRBF_Error(void* object, int error);

static const char sj_build[] =
    "\nSJ/GC Ver.6.25 Build:Sep  3 2004 17:48:17\n";

static const SJRingBufferUuid sjrbf_uuid = {
    0x3B9A9E81,
    0x0DBB,
    0x11D2,
    {0xA6, 0xBF, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
};

SJInterface sjrbf_vtbl = {
    {0, 0, 0},
    SJRBF_Destroy,
    SJRBF_GetUuid,
    SJRBF_Reset,
    SJRBF_GetChunk,
    SJRBF_UngetChunk,
    SJRBF_PutChunk,
    SJRBF_GetNumData,
    SJRBF_IsGetChunk,
    SJRBF_EntryErrFunc
};

int sjrbf_init_cnt = 0;
SJRingBuffer sjrbf_obj[256];

int SJRBF_GetFlowCnt(SJ* sj, int channel, int counter)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    int flow_count;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090229 : NULL pointer is specified.");
        flow_count = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090230 : Specified handle is invalid.");
        flow_count = 0;
    } else {
        flow_count = ring->flow_count[channel][counter];
    }
    SJCRS_Unlock();

    return flow_count;
}

/* Diagnostics retained from a dead-stripped accessor in the retail object. */
static const char sjrbf_unused_null_error[] =
    "E2004090227 : NULL pointer is specified.";
static const char sjrbf_unused_invalid_error[] =
    "E2004090228 : Specified handle is invalid.";

int SJRBF_GetXtrSize(SJ* sj)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    int size;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090225 : NULL pointer is specified.");
        size = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090226 : Specified handle is invalid.");
        size = 0;
    } else {
        size = ring->extra_size;
    }
    SJCRS_Unlock();

    return size;
}

int SJRBF_GetBufSize(SJ* sj)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    int size;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090223 : NULL pointer is specified.");
        size = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090224 : Specified handle is invalid.");
        size = 0;
    } else {
        size = ring->buffer_size;
    }
    SJCRS_Unlock();

    return size;
}

u8* SJRBF_GetBufPtr(SJ* sj)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    u8* buffer;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090221 : NULL pointer is specified.");
        buffer = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090222 : Specified handle is invalid.");
        buffer = 0;
    } else {
        buffer = ring->buffer;
    }
    SJCRS_Unlock();

    return buffer;
}

int SJRBF_IsGetChunk(
    SJ* sj, int channel, int size, int* available)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    int chunk_size;
    int result;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090217 : NULL pointer is specified.");
        result = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090218 : Specified handle is invalid.");
        result = 0;
    } else {
        if (channel == 0) {
            chunk_size = ring->buffer_size - ring->write_position;
            chunk_size += ring->extra_size;
            if (ring->free_size < chunk_size) {
                chunk_size = ring->free_size;
            }
            if (size < chunk_size) {
                chunk_size = size;
            }
        } else if (channel == 1) {
            chunk_size = ring->buffer_size - ring->read_position;
            chunk_size += ring->extra_size;
            if (ring->data_size < chunk_size) {
                chunk_size = ring->data_size;
            }
            if (size < chunk_size) {
                chunk_size = size;
            }
        } else {
            chunk_size = 0;
            if (ring->error_callback != 0) {
                ring->error_callback(ring->error_object, -3);
            }
        }

        *available = chunk_size;
        if (chunk_size != size) {
            result = 0;
        } else {
            result = 1;
        }
    }
    SJCRS_Unlock();

    return result;
}

static void sjrbf_UngetChunk(
    SJRingBuffer* ring, int channel, SJCK* chunk)
{
    int position;
    int chunk_position;

    if (ring == 0) {
        SJERR_CallErr("E2004090215 : NULL pointer is specified.");
        return;
    }
    if (ring->used == 0) {
        SJERR_CallErr("E2004090216 : Specified handle is invalid.");
        return;
    }
    if (ring->buffer_size == 0) {
        SJERR_CallErr("E2004090220 : Illegal buffer size.");
        return;
    }
    if (chunk->len <= 0 || chunk->data == 0) {
        return;
    }

    if (channel == 0) {
        position = (ring->write_position + ring->buffer_size - chunk->len) %
                   ring->buffer_size;
        chunk_position = (chunk->data - ring->buffer) % ring->buffer_size;
        if (position == chunk_position) {
            ring->write_position = position;
            ring->free_size += chunk->len;
        } else if (ring->error_callback != 0) {
            ring->error_callback(ring->error_object, -3);
        }
        ring->flow_count[0][0] -= chunk->len;
    } else if (channel == 1) {
        position = (ring->read_position + ring->buffer_size - chunk->len) %
                   ring->buffer_size;
        chunk_position = (chunk->data - ring->buffer) % ring->buffer_size;
        if (position == chunk_position) {
            ring->read_position = position;
            ring->data_size += chunk->len;
        } else if (ring->error_callback != 0) {
            ring->error_callback(ring->error_object, -3);
        }
        ring->flow_count[1][0] -= chunk->len;
    } else {
        chunk->len = 0;
        chunk->data = 0;
        if (ring->error_callback != 0) {
            ring->error_callback(ring->error_object, -3);
        }
    }
}

void SJRBF_UngetChunk(SJ* sj, int channel, SJCK* chunk)
{
    SJCRS_Lock();
    sjrbf_UngetChunk((SJRingBuffer*)sj, channel, chunk);
    SJCRS_Unlock();
}

void SJRBF_PutChunk(SJ* sj, int channel, SJCK* chunk)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    int offset;
    int length;
    int copy_length;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090213 : NULL pointer is specified.");
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090214 : Specified handle is invalid.");
    } else if (chunk->len > 0 && chunk->data != 0) {
        if (channel == 1) {
            offset = chunk->data - ring->buffer;
            if (offset < ring->extra_size) {
                copy_length = ring->extra_size - offset;
                if (chunk->len < copy_length) {
                    copy_length = chunk->len;
                }
                memcpy(ring->buffer + offset + ring->buffer_size,
                       chunk->data, copy_length);
            }

            length = chunk->data - ring->buffer + chunk->len;
            if (length > ring->buffer_size) {
                copy_length = length - ring->buffer_size;
                if (chunk->len < copy_length) {
                    copy_length = chunk->len;
                }
                memcpy(ring->buffer,
                       ring->buffer + (length - copy_length), copy_length);
            }

            ring->data_size += chunk->len;
            ring->flow_count[1][1] += chunk->len;
        } else if (channel == 0) {
            ring->free_size += chunk->len;
            ring->flow_count[0][1] += chunk->len;
        } else {
            chunk->len = 0;
            chunk->data = 0;
            if (ring->error_callback != 0) {
                ring->error_callback(ring->error_object, -3);
            }
        }
    }
    SJCRS_Unlock();
}

static void sjrbf_GetChunk(
    SJRingBuffer* ring, int channel, int max_size, SJCK* chunk)
{
    int chunk_size;

    if (ring == 0) {
        SJERR_CallErr("E2004090211 : NULL pointer is specified.");
        return;
    }
    if (ring->used == 0) {
        SJERR_CallErr("E2004090212 : Specified handle is invalid.");
        return;
    }
    if (ring->buffer_size == 0) {
        SJERR_CallErr("E2004090219 : Illegal buffer size.");
        return;
    }

    if (channel == 0) {
        chunk_size = ring->buffer_size - ring->write_position;
        chunk_size += ring->extra_size;
        if (ring->free_size < chunk_size) {
            chunk_size = ring->free_size;
        }
        chunk->len = chunk_size;
        if (chunk->len < max_size) {
            max_size = chunk->len;
        }
        chunk->len = max_size;
        chunk->data = ring->buffer + ring->write_position;
        ring->write_position =
            (ring->write_position + chunk->len) % ring->buffer_size;
        ring->free_size -= chunk->len;
        ring->flow_count[0][0] += chunk->len;
    } else if (channel == 1) {
        chunk_size = ring->buffer_size - ring->read_position;
        chunk_size += ring->extra_size;
        if (ring->data_size < chunk_size) {
            chunk_size = ring->data_size;
        }
        chunk->len = chunk_size;
        if (chunk->len < max_size) {
            max_size = chunk->len;
        }
        chunk->len = max_size;
        chunk->data = ring->buffer + ring->read_position;
        ring->read_position =
            (ring->read_position + chunk->len) % ring->buffer_size;
        ring->data_size -= chunk->len;
        ring->flow_count[1][0] += chunk->len;
    } else {
        chunk->len = 0;
        chunk->data = 0;
        if (ring->error_callback != 0) {
            ring->error_callback(ring->error_object, -3);
        }
    }
}

void SJRBF_GetChunk(
    SJ* sj, int channel, int max_size, SJCK* chunk)
{
    SJCRS_Lock();
    sjrbf_GetChunk((SJRingBuffer*)sj, channel, max_size, chunk);
    SJCRS_Unlock();
}

int SJRBF_GetNumData(SJ* sj, int channel)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    int size;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090209 : NULL pointer is specified.");
        size = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090210 : Specified handle is invalid.");
        size = 0;
    } else if (channel == 1) {
        size = ring->data_size;
    } else if (channel == 0) {
        size = ring->free_size;
    } else {
        if (ring->error_callback != 0) {
            ring->error_callback(ring->error_object, -3);
        }
        size = 0;
    }
    SJCRS_Unlock();

    return size;
}

void SJRBF_Reset(SJ* sj)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090207 : NULL pointer is specified.");
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090208 : Specified handle is invalid.");
    } else {
        ring->data_size = 0;
        ring->free_size = ring->buffer_size;
        ring->write_position = 0;
        ring->read_position = 0;
        ring->flow_count[0][0] = 0;
        ring->flow_count[0][1] = 0;
        ring->flow_count[1][0] = 0;
        ring->flow_count[1][1] = 0;
    }
    SJCRS_Unlock();
}

void SJRBF_EntryErrFunc(
    SJ* sj, SJErrorCallback callback, void* object)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090205 : NULL pointer is specified.");
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090206 : Specified handle is invalid.");
    } else {
        ring->error_callback = callback;
        ring->error_object = object;
    }
    SJCRS_Unlock();
}

const void* SJRBF_GetUuid(SJ* sj)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;
    const SJRingBufferUuid* uuid;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090203 : NULL pointer is specified.");
        uuid = 0;
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090204 : Specified handle is invalid.");
        uuid = 0;
    } else {
        uuid = ring->uuid;
    }
    SJCRS_Unlock();

    return uuid;
}

void SJRBF_Destroy(SJ* sj)
{
    SJRingBuffer* ring = (SJRingBuffer*)sj;

    SJCRS_Lock();
    if (ring == 0) {
        SJERR_CallErr("E2004090201 : NULL pointer is specified.");
    } else if (ring->used == 0) {
        SJERR_CallErr("E2004090202 : Specified handle is invalid.");
    } else {
        memset(ring, 0, sizeof(*ring));
        ring->used = 0;
    }
    SJCRS_Unlock();
}

SJ* SJRBF_Create(void* buffer, int buffer_size, int extra_size)
{
    SJRingBuffer* ring;
    int index;

    SJCRS_Lock();
    for (index = 0; index < 256; index++) {
        if (sjrbf_obj[index].used == 0) {
            break;
        }
    }

    if (index == 256) {
        ring = 0;
    } else {
        ring = &sjrbf_obj[index];
        ring->used = 1;
        ring->interface = &sjrbf_vtbl;
        ring->buffer = buffer;
        ring->buffer_size = buffer_size;
        ring->extra_size = extra_size;
        ring->uuid = &sjrbf_uuid;
        ring->error_callback = SJRBF_Error;
        ring->error_object = ring;

        /* Creation performs the validated reset sequence while already locked. */
        if (ring == 0) {
            SJERR_CallErr("E2004090207 : NULL pointer is specified.");
        } else if (ring->used == 0) {
            SJERR_CallErr("E2004090208 : Specified handle is invalid.");
        } else {
            ring->data_size = 0;
            ring->free_size = ring->buffer_size;
            ring->write_position = 0;
            ring->read_position = 0;
            ring->flow_count[0][0] = 0;
            ring->flow_count[0][1] = 0;
            ring->flow_count[1][0] = 0;
            ring->flow_count[1][1] = 0;
        }
    }
    SJCRS_Unlock();

    return (SJ*)ring;
}

void SJRBF_Finish(void)
{
    SJCRS_Lock();
    sjrbf_init_cnt--;
    if (sjrbf_init_cnt == 0) {
        memset(sjrbf_obj, 0, sizeof(sjrbf_obj));
    }
    SJCRS_Unlock();
}

void SJRBF_Init(void)
{
    SJCRS_Lock();
    if (sjrbf_init_cnt == 0) {
        memset(sjrbf_obj, 0, sizeof(sjrbf_obj));
    }
    sjrbf_init_cnt++;
    SJCRS_Unlock();
}

void SJRBF_Error(void* object, int error)
{
    (void)object;
    (void)error;
    SJERR_CallErr("SJRBF Error");
}

/* Retail split-layout tail for the zero-initialized section. */
u32 gap_06_804C718C_bss;
