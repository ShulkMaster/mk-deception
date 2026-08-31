#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct ADXStream ADXStream;

typedef struct LSCStreamInfo {
    s32 id;
    const char* filename;
    u32 filename_checksum;
    void* directory;
    s32 offset;
    s32 sector_count;
    s32 state;
    s32 read_sectors;
} LSCStreamInfo;

typedef struct LSCObject {
    s8 used;
    s8 state;
    s8 reading;
    s8 loop;
    s8 paused;
    u8 reserved_05;
    u16 reserved_06;
    SJ* sj;
    SJCK chunk;
    s32 minimum_buffer_size;
    s32 buffer_size;
    s32 write_position;
    s32 read_position;
    s32 stream_count;
    ADXStream* stream;
    s32 file_sectors;
    s32 requested_sectors;
    s32 error_count;
    LSCStreamInfo stream_info[16];
} LSCObject;

typedef void (*LSCStatusCallback)(void* object1, void* object2);

typedef char LSCStreamInfoSizeCheck[sizeof(LSCStreamInfo) == 0x20 ? 1 : -1];
typedef char LSCObjectSizeCheck[sizeof(LSCObject) == 0x238 ? 1 : -1];

extern void LSC_CallErrFunc(const char* format, ...);
extern void LSC_LockCrs(s32* state);
extern void LSC_UnlockCrs(s32* state);
extern void lsc_ExecHndl(LSCObject* lsc);
extern void ADXSTM_Stop(ADXStream* stream);

extern LSCObject lsc_obj[32];

LSCStatusCallback lsc_stat_func;
void* lsc_stat_obj1;
void* lsc_stat_obj2;
u32 gap_06_804BE5EC_bss;

static inline void lsc_ResetEntries(LSCObject* lsc)
{
    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
    } else if (lsc->state == 0) {
        lsc->write_position = 0;
        lsc->read_position = 0;
        lsc->stream_count = 0;
    }
}

void LSC_SetLpFlg(LSCObject* lsc, s32 loop)
{
    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return;
    }
    lsc->loop = loop;
}

void LSC_CallStatFunc(void)
{
    if (lsc_stat_func != 0) {
        lsc_stat_func(lsc_stat_obj1, lsc_stat_obj2);
    }
}

void LSC_SetFlowLimit(LSCObject* lsc, s32 minimum_buffer_size)
{
    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return;
    }
    if (minimum_buffer_size < 0 ||
        minimum_buffer_size > lsc->buffer_size) {
        LSC_CallErrFunc("E0010: Illigal parameter min_val=%d\n",
                        minimum_buffer_size);
        return;
    }
    lsc->minimum_buffer_size = minimum_buffer_size;
}

s32 LSC_GetNumStm(LSCObject* lsc)
{
    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return -1;
    }
    return lsc->stream_count;
}

s32 LSC_GetStat(LSCObject* lsc)
{
    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return -1;
    }
    return lsc->state;
}

void LSC_ExecServer(void)
{
    LSCObject* lsc;
    s32 critical_state;
    s32 i;

    LSC_LockCrs(&critical_state);
    i = 0;
    lsc = lsc_obj;
    do {
        if (lsc->used == 1) {
            lsc_ExecHndl(lsc);
        }
        i++;
        lsc++;
    } while (i < 32);
    LSC_UnlockCrs(&critical_state);
}

void LSC_Stop(LSCObject* lsc)
{
    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return;
    }
    if (lsc->state != 0) {
        lsc->state = 0;
        if (lsc->stream != 0 && lsc->reading == 1) {
            ADXSTM_Stop(lsc->stream);
            lsc->reading = 0;
        }
        lsc->file_sectors = 0;
        lsc_ResetEntries(lsc);
        lsc->error_count = 0;
    }
}

void LSC_Start(LSCObject* lsc)
{
    s32 critical_state;

    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return;
    }
    LSC_LockCrs(&critical_state);
    if (lsc->state != 0) {
        LSC_Stop(lsc);
    }
    if (lsc->stream_count > 0) {
        lsc->state = 2;
    } else {
        lsc->state = 1;
    }
    LSC_UnlockCrs(&critical_state);
}

s32 LSC_EntryFileRange(LSCObject* lsc, const char* filename,
                       void* directory, s32 offset, s32 sector_count)
{
    LSCStreamInfo* info;
    LSCStreamInfo* previous;
    unsigned long filename_length;
    unsigned long i;
    s32 stream_id;

    if (lsc == 0) {
        LSC_CallErrFunc("E0003: Illigal parameter lsc=NULL\n");
        return -1;
    }
    if (lsc->stream_count >= 16) {
        return -1;
    }
    if (filename == 0) {
        LSC_CallErrFunc("E0011: Illigal parameter fname=%s\n", filename);
        return -1;
    }

    info = &lsc->stream_info[lsc->write_position];
    previous = &lsc->stream_info[(lsc->write_position + 15) % 16];
    if (previous->id == 0x7FFFFFFF) {
        stream_id = 0;
    } else {
        stream_id = previous->id + 1;
    }
    info->id = stream_id;
    info->filename = filename;
    filename_length = strlen(filename);
    info->filename_checksum = 0;
    for (i = 0; i < filename_length; i++) {
        info->filename_checksum += (u8)filename[i];
    }
    info->offset = offset;
    info->sector_count = sector_count;
    info->directory = directory;
    info->state = 0;
    info->read_sectors = 0;

    lsc->stream_count++;
    lsc->write_position = (lsc->write_position + 1) % 16;
    if (lsc->state == 1) {
        lsc->state = 2;
    }
    return stream_id;
}

s32 LSC_EntryFname(LSCObject* lsc, const char* filename)
{
    return LSC_EntryFileRange(lsc, filename, 0, 0, 0xFFFFF);
}

void LSC_SetStmHndl(LSCObject* lsc, ADXStream* stream)
{
    lsc->stream = stream;
}

void LSC_Destroy(LSCObject* lsc)
{
    if (lsc != 0) {
        LSC_Stop(lsc);
        lsc->used = 0;
        memset(lsc, 0, sizeof(LSCObject));
    }
}

LSCObject* LSC_Create(SJ* sj)
{
    LSCObject* lsc;
    LSCObject* current;
    s32 critical_state;
    s32 channel_size;
    s32 i;

    if (sj == 0) {
        LSC_CallErrFunc("E0001: Illigal parameter=sj (LSC_Create)\n");
        return 0;
    }

    LSC_LockCrs(&critical_state);
    lsc = 0;
    current = lsc_obj;
    for (i = 0; i < 32; i++) {
        if (current->used == 0) {
            lsc = &lsc_obj[i];
            break;
        }
        current++;
    }

    if (lsc == 0) {
        LSC_CallErrFunc("E0002: Not enough instance (LSC_Create)\n");
    } else {
        lsc->sj = sj;
        lsc->state = 0;
        channel_size = sj->interface->get_num_data(sj, 1);
        lsc->buffer_size =
            sj->interface->get_num_data(sj, 0) + channel_size;
        lsc->minimum_buffer_size = (lsc->buffer_size * 8) / 10;
        for (i = 0; i < 16; i++) {
            lsc->stream_info[i].state = 0;
        }
        lsc->used = 1;
    }
    LSC_UnlockCrs(&critical_state);
    return lsc;
}
