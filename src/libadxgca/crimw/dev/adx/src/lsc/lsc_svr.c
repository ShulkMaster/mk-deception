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

typedef char LSCStreamInfoSizeCheck[sizeof(LSCStreamInfo) == 0x20 ? 1 : -1];
typedef char LSCObjectSizeCheck[sizeof(LSCObject) == 0x238 ? 1 : -1];

extern void LSC_CallErrFunc(const char* format, ...);
extern void LSC_CallStatFunc(void);
extern s32 LSC_EntryFileRange(LSCObject* lsc, const char* filename,
                              void* directory, s32 offset, s32 sector_count);

extern s32 ADXSTM_GetStat(ADXStream* stream);
extern s32 ADXSTM_Tell(ADXStream* stream);
extern void ADXSTM_StopNw(ADXStream* stream);
extern void ADXSTM_ReleaseFileNw(ADXStream* stream);
extern void ADXSTM_BindFileNw(ADXStream* stream, const char* filename,
                              void* directory, s32 offset, s32 sector_count);
extern void ADXSTM_SetEos(ADXStream* stream, s32 sector_count);
extern s32 ADXSTM_SetBufSize(ADXStream* stream, s32 minimum_size,
                             s32 buffer_size);
extern void ADXSTM_Seek(ADXStream* stream, s32 position);
extern void ADXSTM_Start(ADXStream* stream);

static inline void lsc_StatRead(LSCObject* lsc)
{
    LSCStreamInfo* info;
    s32 state;

    if (lsc->stream == 0) {
        LSC_CallErrFunc("E0007: lsc->fp=NULL\n");
        return;
    }

    info = &lsc->stream_info[lsc->read_position];
    state = ADXSTM_GetStat(lsc->stream);
    switch (state) {
    case 4:
        lsc->state = 3;
        break;
    case 2:
        info->read_sectors = ADXSTM_Tell(lsc->stream);
        break;
    case 3:
        info->read_sectors = lsc->file_sectors;
        info->state = 2;
        break;
    }
}

static inline void lsc_StatEnd(LSCObject* lsc)
{
    const char* filename = 0;
    void* directory = 0;
    s32 offset = 0;
    s32 sector_count = 0;
    LSCStreamInfo* info;

    if (lsc->stream != 0) {
        if (lsc->loop == 1) {
            info = &lsc->stream_info[lsc->read_position];
            filename = info->filename;
            directory = info->directory;
            offset = info->offset;
            sector_count = info->sector_count;
        }

        lsc->stream_count--;
        lsc->read_position = (lsc->read_position + 1) % 16;

        if (lsc->stream_count <= 0) {
            LSC_CallStatFunc();
            lsc->state = 1;
        }

        if (lsc->loop == 1) {
            LSC_EntryFileRange(lsc, filename, directory, offset, sector_count);
        }
    }
}

static inline u32 lsc_GetFilenameChecksum(const char* filename)
{
    unsigned long length;
    u32 checksum;
    unsigned long index;

    length = strlen(filename);
    checksum = 0;
    for (index = 0; index < length; index++) {
        checksum += (u8)filename[index];
    }
    return checksum;
}

static inline void lsc_StatWait(LSCObject* lsc)
{
    LSCStreamInfo* info;
    u32 filename_checksum;

    info = &lsc->stream_info[lsc->read_position];
    if (lsc->stream_count <= 0) {
        return;
    }

    ADXSTM_StopNw(lsc->stream);
    ADXSTM_ReleaseFileNw(lsc->stream);

    filename_checksum = lsc_GetFilenameChecksum(info->filename);
    if (filename_checksum != info->filename_checksum) {
        LSC_CallErrFunc(
            "E0013: '%s' is different from entry file name.(LSC_ExecServer)\n",
            info->filename);
        return;
    }

    ADXSTM_BindFileNw(lsc->stream, info->filename, info->directory,
                      info->offset, info->sector_count);
    ADXSTM_SetEos(lsc->stream, info->sector_count);
    lsc->file_sectors = info->sector_count;
    info->read_sectors = 0;
    lsc->reading = 0;

    if (lsc->reading == 0) {
        ADXSTM_SetBufSize(lsc->stream, lsc->minimum_buffer_size,
                          lsc->buffer_size);
        ADXSTM_Seek(lsc->stream, 0);
        ADXSTM_Start(lsc->stream);
        lsc->reading = 1;
    }

    info->state = 1;
}

void lsc_ExecHndl(LSCObject* lsc)
{
    if (lsc->paused == 1) {
        return;
    }
    if (lsc->state != 2) {
        return;
    }
    if (lsc->stream_count <= 0) {
        return;
    }

    if (lsc->stream_info[lsc->read_position].state == 1) {
        lsc_StatRead(lsc);
    }

    if (lsc->stream_info[lsc->read_position].state == 2) {
        lsc_StatEnd(lsc);
    }

    if (lsc->stream_info[lsc->read_position].state != 0) {
        return;
    }

    lsc_StatWait(lsc);
}
