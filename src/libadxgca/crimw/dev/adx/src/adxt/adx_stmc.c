#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

enum {
    ADXSTM_STATUS_STOP = 1,
    ADXSTM_STATUS_READING = 2,
    ADXSTM_STATUS_END = 3,
    ADXSTM_STATUS_ERROR = 4,
    ADXSTM_MAX_SECTORS = 0xFFFFF,
    ADXSTM_SECTOR_SIZE = 0x800,
    ADXSTM_NUM_OBJECTS = 40
};

typedef struct CvFsObject CvFsObject;
typedef struct ADXStream ADXStream;
typedef void (*ADXStreamEosCallback)(void* object);

struct ADXStream {
    s8 used;
    s8 status;
    s8 read_active;
    s8 retry_count;
    SJ* sj;
    CvFsObject* file;
    s32 file_offset;
    s32 file_size;
    s32 file_sectors;
    s32 maximum_buffer_size;
    s32 minimum_buffer_size;
    s32 request_sectors;
    SJCK request_chunk;
    s32 maximum_request_sectors;
    s32 eos_sector;
    s32 transferred_bytes;
    ADXStreamEosCallback eos_callback;
    void* eos_object;
    s32 sj_buffer_size;
    s8 stop_requested;
    s8 bind_requested;
    s8 release_requested;
    s8 start_requested;
    s8 stop_pending;
    s8 file_open;
    s8 realtime;
    u8 reserved_4B[5];
    const char* filename;
    void* directory;
    s32 position;
    s32 transfer_limit;
};

typedef char ADXStreamSizeCheck[sizeof(ADXStream) == 0x60 ? 1 : -1];

extern s32 SVM_TestAndSet(s32* value);
extern void SVM_Lock(void);
extern void SVM_Unlock(void);
extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void ADXT_ExecFsSvr(void);
extern void ADXERR_CallErrFunc2(const char* message1, const char* message2);

extern CvFsObject* cvFsOpen(const char* filename, void* directory, s32 mode);
extern void cvFsClose(CvFsObject* file);
extern s32 cvFsSeek(CvFsObject* file, s32 offset, s32 origin);
extern s32 cvFsTell(CvFsObject* file);
extern s32 cvFsGetFileSize(const char* filename);
extern s32 cvFsGetStat(CvFsObject* file);
extern s32 cvFsReqRd(CvFsObject* file, s32 sectors, void* buffer);
extern void cvFsStopTr(CvFsObject* file);

s32 adxstmf_rtim_ofst = 0;
s32 adxstmf_rtim_num = 16;
s32 adxstmf_nrml_ofst = 16;
s32 adxstmf_nrml_num = 24;
s32 adxstmf_execsvr_flg = 0;
s32 adxstmf_num_rtry = 0;
s32 adxstm_sj_internal_error_cnt = 0;
ADXStream adxstmf_obj[ADXSTM_NUM_OBJECTS];

void ADXSTMF_ExecHndl(ADXStream* stream);
void adxstmf_stat_exec(ADXStream* stream);

static inline s32 adxstm_Seek(ADXStream* stream, s32 position)
{
    stream->position = position;
    if (stream->position > stream->file_sectors) {
        stream->position = stream->file_sectors;
    }
    return stream->position;
}

static inline void adxstm_StopNw(ADXStream* stream)
{
    SVM_Lock();
    if (stream->status == ADXSTM_STATUS_READING && stream->read_active == 1) {
        stream->stop_pending = 1;
        if (stream->start_requested == 1) {
            stream->start_requested = 0;
        }
    } else {
        stream->status = ADXSTM_STATUS_STOP;
    }
    SVM_Unlock();
}

static inline void adxstm_ReleaseFileNw(ADXStream* stream)
{
    adxstm_StopNw(stream);

    SVM_Lock();
    if (stream->file_open == 1) {
        stream->release_requested = 1;
    }
    stream->bind_requested = 0;
    SVM_Unlock();
}

s32 ADXSTM_SetBufSize(ADXStream* stream, s32 minimum_size, s32 maximum_size)
{
    stream->minimum_buffer_size = minimum_size;
    stream->maximum_buffer_size = maximum_size;
    return 1;
}

void ADXSTM_ExecServer(void)
{
    s32 index;

    if (SVM_TestAndSet(&adxstmf_execsvr_flg) != 0) {
        for (index = 0; index < ADXSTM_NUM_OBJECTS; index++) {
            if (adxstmf_obj[index].used == 1) {
                ADXSTMF_ExecHndl(&adxstmf_obj[index]);
            }
        }
        adxstmf_execsvr_flg = 0;
    }
}

void ADXSTMF_ExecHndl(ADXStream* stream)
{
    CvFsObject* file;
    s32 file_sectors;
    s32 file_size;

    if (stream->read_active == 0) {
        if (stream->stop_pending == 1) {
            stream->stop_pending = 0;
            if (stream->start_requested == 0) {
                stream->status = ADXSTM_STATUS_STOP;
            }
        }

        if (stream->release_requested == 1) {
            CvFsObject* file = stream->file;

            if (file != 0) {
                stream->file = 0;
                cvFsClose(file);
            }
            stream->release_requested = 0;
            stream->file_open = 0;
        }

        SVM_Lock();
        if (stream->bind_requested == 1) {
            stream->file_open = 1;
            SVM_Unlock();

            if (stream->file == 0) {
                file = cvFsOpen(stream->filename, stream->directory, 0);
                stream->file = file;
                if (file == 0) {
                    ADXERR_CallErrFunc2(
                        "E02110501 adxstmf_stat_exec: can't open ",
                        stream->filename);
                    stream->status = ADXSTM_STATUS_ERROR;
                    stream->file_open = 0;
                    stream->bind_requested = 0;
                    return;
                }

                cvFsSeek(stream->file, 0, 2);
                file_sectors = cvFsTell(stream->file);
                if (stream->directory == 0) {
                    file_size = cvFsGetFileSize(stream->filename);
                } else {
                    file_size = file_sectors * ADXSTM_SECTOR_SIZE;
                }
                cvFsSeek(stream->file, 0, 0);

                if (stream->file_size ==
                    ADXSTM_MAX_SECTORS * ADXSTM_SECTOR_SIZE) {
                    stream->file_size = file_size;
                    stream->file_sectors = file_sectors;
                } else {
                    if (stream->file_offset > file_sectors) {
                        stream->file_offset = file_sectors;
                    }
                    if (stream->file_sectors + stream->file_offset >
                        file_sectors) {
                        stream->file_sectors = file_sectors - stream->file_offset;
                        stream->file_size =
                            stream->file_sectors * ADXSTM_SECTOR_SIZE;
                    }
                }

                adxstm_Seek(stream, 0);
                stream->bind_requested = 0;
            }
        } else {
            SVM_Unlock();
        }

        if (stream->start_requested == 1) {
            stream->start_requested = 0;
        }
    }

    if (stream->status == ADXSTM_STATUS_READING && stream->file_open == 1) {
        adxstmf_stat_exec(stream);
    }
}

void adxstmf_stat_exec(ADXStream* stream)
{
    SJ* sj = stream->sj;
    s32 file_status = cvFsGetStat(stream->file);
    s32 requested_bytes;
    s32 file_sector_count;
    s32 request_sectors;
    s32 remaining_sectors;
    SJCK completed_chunk;
    SJCK remainder_chunk;
    SJCK read_chunk;

    SVM_Lock();
    if (stream->read_active == 1) {
        if (file_status == 1) {
            stream->read_active = 0;
            SVM_Unlock();

            requested_bytes = stream->request_sectors * ADXSTM_SECTOR_SIZE;
            SJ_SplitChunk(&stream->request_chunk, requested_bytes,
                          &completed_chunk, &remainder_chunk);
            sj->interface->put_chunk(sj, 1, &completed_chunk);
            sj->interface->unget_chunk(sj, 0, &remainder_chunk);

            stream->position += stream->request_sectors;
            stream->transferred_bytes += requested_bytes;
            stream->request_chunk.data = 0;
            stream->request_chunk.len = 0;

            file_sector_count = stream->file_sectors;
            if (stream->position == stream->eos_sector &&
                stream->eos_callback != 0) {
                stream->eos_callback(stream->eos_object);
            }

            if (stream->position >= file_sector_count) {
                stream->status = ADXSTM_STATUS_END;
            } else if ((u32)stream->transferred_bytes / ADXSTM_SECTOR_SIZE >=
                           (u32)stream->transfer_limit &&
                       (u32)stream->transfer_limit < ADXSTM_MAX_SECTORS) {
                stream->status = ADXSTM_STATUS_END;
            }
            stream->retry_count = 0;
            return;
        }

        if (file_status == 3) {
            stream->read_active = 0;
            SVM_Unlock();
            sj->interface->unget_chunk(sj, 0, &stream->request_chunk);
            stream->request_chunk.data = 0;
            stream->request_chunk.len = 0;

            if (adxstmf_num_rtry >= 0) {
                if (stream->retry_count >= adxstmf_num_rtry) {
                    stream->status = ADXSTM_STATUS_ERROR;
                } else {
                    stream->retry_count++;
                }
            }
            return;
        }

        SVM_Unlock();
        return;
    }

    stream->read_active = 1;
    stream->request_chunk.data = 0;
    stream->request_chunk.len = 0;
    SVM_Unlock();

    if (stream->stop_requested == 1 || stream->stop_pending == 1) {
        stream->read_active = 0;
        return;
    }

    if (stream->file_sectors == 0) {
        stream->read_active = 0;
        stream->request_sectors = 0;
        stream->status = ADXSTM_STATUS_END;
        return;
    }

    if (sj == 0 || sj->interface == 0) {
        stream->read_active = 0;
        adxstm_sj_internal_error_cnt++;
        return;
    }

    if (stream->sj_buffer_size - sj->interface->get_num_data(sj, 0) >=
        stream->minimum_buffer_size) {
        stream->read_active = 0;
        return;
    }

    sj->interface->get_chunk(sj, 0, stream->maximum_buffer_size, &read_chunk);

    request_sectors = read_chunk.len / ADXSTM_SECTOR_SIZE;
    remaining_sectors = stream->eos_sector - stream->position;
    if (request_sectors < remaining_sectors) {
        remaining_sectors = request_sectors;
    }

    request_sectors = stream->file_sectors - stream->position;
    if (remaining_sectors < request_sectors) {
        request_sectors = remaining_sectors;
    }
    if (request_sectors < stream->maximum_request_sectors) {
        remaining_sectors = request_sectors;
    } else {
        remaining_sectors = stream->maximum_request_sectors;
    }

    cvFsSeek(stream->file, stream->file_offset + stream->position, 0);

    request_sectors = stream->transfer_limit -
                      stream->transferred_bytes / ADXSTM_SECTOR_SIZE;
    if (remaining_sectors < request_sectors) {
        request_sectors = remaining_sectors;
    }

    stream->request_sectors =
        cvFsReqRd(stream->file, request_sectors, read_chunk.data);
    stream->request_chunk.data = read_chunk.data;
    stream->request_chunk.len = read_chunk.len;
    if (stream->request_sectors <= 0) {
        sj->interface->unget_chunk(sj, 0, &stream->request_chunk);
        stream->request_chunk.data = 0;
        stream->request_chunk.len = 0;
        stream->read_active = 0;

        if (cvFsGetStat(stream->file) == 3 && adxstmf_num_rtry >= 0) {
            if (stream->retry_count >= adxstmf_num_rtry) {
                stream->status = ADXSTM_STATUS_ERROR;
            } else {
                stream->retry_count++;
            }
        }
    }
}

void ADXSTM_SetEos(ADXStream* stream, s32 sector_count)
{
    if (sector_count >= 0) {
        stream->eos_sector = sector_count;
    } else {
        stream->eos_sector = stream->file_sectors;
    }
}

void ADXSTM_EntryEosFunc(ADXStream* stream, ADXStreamEosCallback callback,
                         void* object)
{
    stream->eos_callback = callback;
    stream->eos_object = object;
}

void ADXSTM_Stop(ADXStream* stream)
{
    if (stream->file != 0 && stream->release_requested == 0) {
        cvFsStopTr(stream->file);
    }

    SVM_Lock();
    stream->status = ADXSTM_STATUS_STOP;
    stream->read_active = 0;
    stream->request_chunk.data = 0;
    SVM_Unlock();

    adxstm_StopNw(stream);
    do {
        ADXT_ExecFsSvr();
    } while (stream->status != ADXSTM_STATUS_STOP ||
             stream->request_chunk.data != 0);
}

void ADXSTM_StopNw(ADXStream* stream)
{
    adxstm_StopNw(stream);
}

s32 ADXSTM_Start(ADXStream* stream)
{
    ADXCRS_Lock();
    stream->transferred_bytes = 0;
    stream->retry_count = 0;
    if (stream->file_sectors == 0) {
        stream->status = ADXSTM_STATUS_END;
    } else {
        stream->status = ADXSTM_STATUS_READING;
    }
    stream->read_active = 0;
    stream->request_chunk.data = 0;
    stream->request_chunk.len = 0;
    stream->start_requested = 1;
    stream->transfer_limit = ADXSTM_MAX_SECTORS;
    ADXCRS_Unlock();
    return 1;
}

s32 ADXSTM_Tell(ADXStream* stream)
{
    if (stream->file != 0) {
        return stream->position;
    }
    return 0;
}

s32 ADXSTM_Seek(ADXStream* stream, s32 position)
{
    return adxstm_Seek(stream, position);
}

s32 ADXSTM_GetStat(ADXStream* stream)
{
    return stream->status;
}

void ADXSTM_ReleaseFile(ADXStream* stream)
{
    ADXSTM_Stop(stream);
    adxstm_ReleaseFileNw(stream);
    for (;;) {
        if (stream->file_open == 0) {
            break;
        }
        ADXT_ExecFsSvr();
    }
}

void ADXSTM_ReleaseFileNw(ADXStream* stream)
{
    adxstm_ReleaseFileNw(stream);
}

void ADXSTM_BindFileNw(ADXStream* stream, const char* filename,
                       void* directory, s32 file_offset, s32 file_sectors)
{
    SVM_Lock();
    stream->file_offset = file_offset;
    stream->file_size = file_sectors * ADXSTM_SECTOR_SIZE;
    stream->file_sectors = file_sectors;
    stream->filename = filename;
    stream->directory = directory;
    stream->bind_requested = 1;
    SVM_Unlock();
}

void ADXSTM_Destroy(ADXStream* stream)
{
    if (stream != 0) {
        ADXSTM_Stop(stream);
        ADXSTM_ReleaseFile(stream);
        stream->used = 0;
        memset(stream, 0, sizeof(*stream));
    }
}

ADXStream* ADXSTM_Create(SJ* sj, s32 priority)
{
    ADXStream* stream;
    s32 index;
    s32 sj_size;
    s32* realtime_offset;

    if (priority < 0x100) {
        realtime_offset = &adxstmf_rtim_ofst;
        stream = 0;
        for (index = 0; index < adxstmf_rtim_num; index++) {
            stream = &adxstmf_obj[*realtime_offset + index];
            if (stream->used == 0) {
                break;
            }
        }

        if (index == adxstmf_rtim_num) {
            stream = 0;
        } else {
            ADXCRS_Lock();
            stream->status = ADXSTM_STATUS_STOP;
            stream->read_active = 0;
            stream->sj = sj;
            stream->file = 0;
            stream->file_offset = 0;
            stream->file_size = 0;
            stream->file_sectors = 0;
            stream->maximum_request_sectors = 0x200;
            stream->position = 0;
            stream->transfer_limit = ADXSTM_MAX_SECTORS;
            stream->eos_sector = stream->file_sectors;
            if (stream->sj != 0) {
                sj_size = sj->interface->get_num_data(sj, 1);
                stream->sj_buffer_size =
                    sj_size + sj->interface->get_num_data(sj, 0);
                stream->minimum_buffer_size = stream->maximum_buffer_size =
                    stream->sj_buffer_size;
            }
            stream->stop_requested = 0;
            stream->used = 1;
            ADXCRS_Unlock();
            stream->realtime = 1;
        }
        return stream;
    }

    stream = 0;
    for (index = 0; index < adxstmf_nrml_num; index++) {
        stream = &adxstmf_obj[adxstmf_nrml_ofst + index];
        if (stream->used == 0) {
            break;
        }
    }

    if (index == adxstmf_nrml_num) {
        stream = 0;
    } else {
        ADXCRS_Lock();
        stream->status = ADXSTM_STATUS_STOP;
        stream->read_active = 0;
        stream->sj = sj;
        stream->file = 0;
        stream->file_offset = 0;
        stream->file_size = 0;
        stream->file_sectors = 0;
        stream->maximum_request_sectors = 0x200;
        stream->position = 0;
        stream->transfer_limit = ADXSTM_MAX_SECTORS;
        stream->eos_sector = stream->file_sectors;
        if (stream->sj != 0) {
            sj_size = sj->interface->get_num_data(sj, 1);
            stream->sj_buffer_size =
                sj_size + sj->interface->get_num_data(sj, 0);
            stream->minimum_buffer_size = stream->maximum_buffer_size =
                stream->sj_buffer_size;
        }
        stream->stop_requested = 0;
        stream->used = 1;
        ADXCRS_Unlock();
        stream->realtime = 0;
    }
    return stream;
}

void ADXSTM_Finish(void)
{
}

s32 ADXSTM_Init(void)
{
    memset(adxstmf_obj, 0, sizeof(adxstmf_obj));
    return 1;
}
