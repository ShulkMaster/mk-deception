#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct SJMemoryUuid {
    u32 time_low;
    u16 time_mid;
    u16 time_high_and_version;
    u8 clock_sequence_and_node[8];
} SJMemoryUuid;

typedef struct SJMemory {
    const SJInterface* interface;
    int used;
    const SJMemoryUuid* uuid;
    int available;
    int read_position;
    u8* buffer;
    int buffer_size;
    SJErrorCallback error_callback;
    void* error_object;
} SJMemory;

typedef char SJMemoryUuidSizeCheck[sizeof(SJMemoryUuid) == 0x10 ? 1 : -1];
typedef char SJMemorySizeCheck[sizeof(SJMemory) == 0x24 ? 1 : -1];

extern void SJCRS_Lock(void);
extern void SJCRS_Unlock(void);
extern void SJERR_CallErr(const char* message);

void SJMEM_Destroy(SJ* sj);
const void* SJMEM_GetUuid(SJ* sj);
void SJMEM_Reset(SJ* sj);
void SJMEM_GetChunk(SJ* sj, int channel, int max_size, SJCK* chunk);
void SJMEM_UngetChunk(SJ* sj, int channel, SJCK* chunk);
void SJMEM_PutChunk(SJ* sj, int channel, SJCK* chunk);
int SJMEM_GetNumData(SJ* sj, int channel);
int SJMEM_IsGetChunk(SJ* sj, int channel, int size, int* available);
void SJMEM_EntryErrFunc(SJ* sj, SJErrorCallback callback, void* object);
void SJMEM_Error(void* object, int error);

static const SJMemoryUuid sjmem_uuid = {
    0xDD9EEE41,
    0x1679,
    0x11D2,
    {0x93, 0x6C, 0x00, 0x60, 0x08, 0x94, 0x48, 0xBC}
};

SJInterface sjmem_vtbl = {
    {0, 0, 0},
    SJMEM_Destroy,
    SJMEM_GetUuid,
    SJMEM_Reset,
    SJMEM_GetChunk,
    SJMEM_UngetChunk,
    SJMEM_PutChunk,
    SJMEM_GetNumData,
    SJMEM_IsGetChunk,
    SJMEM_EntryErrFunc
};

int sjmem_init_cnt = 0;
SJMemory sjmem_obj[32];

int SJMEM_GetBufSize(SJ* sj)
{
    SJMemory* memory = (SJMemory*)sj;
    int size;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090251 : NULL pointer is specified.");
        size = 0;
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090252 : Specified handle is invalid.");
        size = 0;
    } else {
        size = memory->buffer_size;
    }
    SJCRS_Unlock();

    return size;
}

/* Retail retains these diagnostics although SJMEM_GetBufPtr was dead-stripped. */
static const char sjmem_get_buf_ptr_null_error[] =
    "E2004090249 : NULL pointer is specified.";
static const char sjmem_get_buf_ptr_invalid_error[] =
    "E2004090250 : Specified handle is invalid.";

int SJMEM_IsGetChunk(SJ* sj, int channel, int size, int* available)
{
    SJMemory* memory = (SJMemory*)sj;
    int obtainable;
    int result;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090247 : NULL pointer is specified.");
        result = 0;
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090248 : Specified handle is invalid.");
        result = 0;
    } else {
        if (channel == 0) {
            obtainable = 0;
        } else if (channel == 1) {
            obtainable = size;
            if (memory->available < size) {
                obtainable = memory->available;
            }
        } else {
            obtainable = 0;
            if (memory->error_callback != 0) {
                memory->error_callback(memory->error_object, -3);
            }
        }

        *available = obtainable;
        if (obtainable != size) {
            result = 0;
        } else {
            result = 1;
        }
    }
    SJCRS_Unlock();

    return result;
}

void SJMEM_UngetChunk(SJ* sj, int channel, SJCK* chunk)
{
    int position;
    int clamped_position;
    int data_position;
    SJMemory* memory;

    memory = (SJMemory*)sj;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090245 : NULL pointer is specified.");
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090246 : Specified handle is invalid.");
    } else if (chunk->len > 0 && chunk->data != 0) {
        if (channel == 0) {
            if (memory->error_callback != 0) {
                memory->error_callback(memory->error_object, -3);
            }
        } else if (channel == 1) {
            position = memory->read_position - chunk->len;
            clamped_position = position > 0 ? position : 0;
            memory->read_position = clamped_position;

            position = memory->available + chunk->len;
            memory->available =
                memory->buffer_size < position ? memory->buffer_size : position;

            data_position = chunk->data - memory->buffer;
            if (clamped_position != data_position &&
                memory->error_callback != 0) {
                memory->error_callback(memory->error_object, -3);
            }
        } else {
            chunk->len = 0;
            chunk->data = 0;
            if (memory->error_callback != 0) {
                memory->error_callback(memory->error_object, -3);
            }
        }
    }
    SJCRS_Unlock();
}

void SJMEM_PutChunk(SJ* sj, int channel, SJCK* chunk)
{
    SJMemory* memory = (SJMemory*)sj;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090243 : NULL pointer is specified.");
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090244 : Specified handle is invalid.");
    } else if (chunk->len > 0 && chunk->data != 0 && channel != 0 &&
               channel != 1) {
        chunk->len = 0;
        chunk->data = 0;
        if (memory->error_callback != 0) {
            memory->error_callback(memory->error_object, -3);
        }
    }
    SJCRS_Unlock();
}

void SJMEM_GetChunk(SJ* sj, int channel, int max_size, SJCK* chunk)
{
    SJMemory* memory = (SJMemory*)sj;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090241 : NULL pointer is specified.");
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090242 : Specified handle is invalid.");
    } else if (channel == 0) {
        chunk->len = 0;
        chunk->data = 0;
    } else if (channel == 1) {
        chunk->len = memory->available < max_size ? memory->available
                                                  : max_size;
        chunk->data = memory->buffer + memory->read_position;
        memory->read_position += chunk->len;
        memory->available -= chunk->len;
    } else {
        chunk->len = 0;
        chunk->data = 0;
        if (memory->error_callback != 0) {
            memory->error_callback(memory->error_object, -3);
        }
    }
    SJCRS_Unlock();
}

int SJMEM_GetNumData(SJ* sj, int channel)
{
    SJMemory* memory = (SJMemory*)sj;
    int available;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090239 : NULL pointer is specified.");
        available = 0;
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090240 : Specified handle is invalid.");
        available = 0;
    } else if (channel == 1) {
        available = memory->available;
    } else if (channel == 0) {
        available = 0;
    } else {
        if (memory->error_callback != 0) {
            memory->error_callback(memory->error_object, -3);
        }
        available = 0;
    }
    SJCRS_Unlock();

    return available;
}

void SJMEM_Reset(SJ* sj)
{
    SJMemory* memory = (SJMemory*)sj;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090237 : NULL pointer is specified.");
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090238 : Specified handle is invalid.");
    } else {
        memory->available = memory->buffer_size;
        memory->read_position = 0;
    }
    SJCRS_Unlock();
}

void SJMEM_EntryErrFunc(SJ* sj, SJErrorCallback callback, void* object)
{
    SJMemory* memory = (SJMemory*)sj;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090235 : NULL pointer is specified.");
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090236 : Specified handle is invalid.");
    } else {
        memory->error_callback = callback;
        memory->error_object = object;
    }
    SJCRS_Unlock();
}

const void* SJMEM_GetUuid(SJ* sj)
{
    SJMemory* memory = (SJMemory*)sj;
    const SJMemoryUuid* uuid;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090233 : NULL pointer is specified.");
        uuid = 0;
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090234 : Specified handle is invalid.");
        uuid = 0;
    } else {
        uuid = memory->uuid;
    }
    SJCRS_Unlock();

    return uuid;
}

void SJMEM_Destroy(SJ* sj)
{
    SJMemory* memory = (SJMemory*)sj;

    SJCRS_Lock();
    if (memory == 0) {
        SJERR_CallErr("E2004090231 : NULL pointer is specified.");
    } else if (memory->used == 0) {
        SJERR_CallErr("E2004090232 : Specified handle is invalid.");
    } else {
        memset(memory, 0, sizeof(*memory));
        memory->used = 0;
    }
    SJCRS_Unlock();
}

SJ* SJMEM_Create(void* buffer, int buffer_size)
{
    int index;
    SJMemory* memory;

    SJCRS_Lock();
    for (index = 0; index < 32; index++) {
        if (sjmem_obj[index].used == 0) {
            break;
        }
    }

    if (index == 32) {
        memory = 0;
    } else {
        memory = &sjmem_obj[index];
        memory->used = 1;
        memory->interface = &sjmem_vtbl;
        memory->buffer = buffer;
        memory->buffer_size = buffer_size;
        memory->uuid = &sjmem_uuid;
        memory->error_callback = SJMEM_Error;
        memory->error_object = memory;

        /* Creation performs the same validated reset sequence as SJMEM_Reset. */
        if (memory == 0) {
            SJERR_CallErr("E2004090237 : NULL pointer is specified.");
        } else if (memory->used == 0) {
            SJERR_CallErr("E2004090238 : Specified handle is invalid.");
        } else {
            memory->available = memory->buffer_size;
            memory->read_position = 0;
        }
    }
    SJCRS_Unlock();

    return (SJ*)memory;
}

void SJMEM_Finish(void)
{
    SJCRS_Lock();
    sjmem_init_cnt--;
    if (sjmem_init_cnt == 0) {
        memset(sjmem_obj, 0, sizeof(sjmem_obj));
    }
    SJCRS_Unlock();
}

void SJMEM_Init(void)
{
    SJCRS_Lock();
    if (sjmem_init_cnt == 0) {
        memset(sjmem_obj, 0, sizeof(sjmem_obj));
    }
    sjmem_init_cnt++;
    SJCRS_Unlock();
}

void SJMEM_Error(void* object, int error)
{
    (void)object;
    (void)error;
    SJERR_CallErr("SJMEM Error");
}

/* Retail split-layout tails for the read-only and zero-initialized sections. */
const u32 gap_04_8031C3F4_rodata = 0;
u32 gap_06_804C3184_bss;
