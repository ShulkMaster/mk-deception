#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct ADXStream ADXStream;

typedef struct ADXFCommandRecord {
    u8 command;
    u8 phase;
    u16 sequence;
    void* file;
    s32 position;
    s32 length;
} ADXFCommandRecord;

typedef struct ADXFFile {
    s8 used;
    s8 state;
    s8 sjflag;
    s8 stop_requested;
    ADXStream* stm;
    SJ* sj;
    u8 reserved_0C[8];
    s32 skpos;
    u8 reserved_18[8];
    s32 rdsct;
    void* buf;
    s32 bsize;
    u8 reserved_2C[0x18];
} ADXFFile;

typedef char ADXFCommandRecordSizeCheck[
    sizeof(ADXFCommandRecord) == 0x10 ? 1 : -1];
typedef char ADXFFileSizeCheck[sizeof(ADXFFile) == 0x44 ? 1 : -1];

extern s32 adxf_hstry_no;
extern u16 adxf_cmd_ncall[16];
extern ADXFCommandRecord adxf_cmd_hstry[16];
extern s32 adxf_ocbi_fg;
extern ADXFFile adxf_obj[16];

extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern s32 ADXSTM_GetStat(ADXStream* stream);
extern s32 ADXSTM_Tell(ADXStream* stream);
extern void ADXSTM_Stop(ADXStream* stream);
extern void ADXSTM_ReleaseFile(ADXStream* stream);
extern void ADXSTM_Destroy(ADXStream* stream);
extern void ADXF_Ocbi(void* address, s32 size);
extern void ADXERR_CallErrFunc1(const char* message);

static inline void adxf_SetCmdHstry(s32 command, s32 phase, ADXFFile* file,
                                    s32 position, s32 length)
{
    ADXFCommandRecord* history;

    adxf_hstry_no %= 16;
    history = &adxf_cmd_hstry[adxf_hstry_no];
    if (phase == 0) {
        adxf_cmd_ncall[command]++;
    }

    history->command = command;
    history->phase = phase;
    history->sequence = adxf_cmd_ncall[command];
    history->file = file;
    history->position = position;
    history->length = length;
    adxf_hstry_no++;
}

static inline void adxf_CloseSjStm(ADXFFile* file)
{
    SJ* stream;

    if (file->sj != 0 && file->sjflag == 0) {
        if (adxf_ocbi_fg == 1) {
            ADXF_Ocbi(file->buf, file->bsize);
        }

        stream = file->sj;
        file->sj = 0;
        stream->interface->destroy(stream);
    }
}

static inline void adxf_ExecOne(ADXFFile* file)
{
    if (file->state == 2) {
        file->state = ADXSTM_GetStat(file->stm);
        file->rdsct = ADXSTM_Tell(file->stm) - file->skpos;

        if (file->state == 3 || file->state == 4) {
            file->skpos += file->rdsct;
            adxf_CloseSjStm(file);
        }
    }

    if (file->stop_requested == 1 && ADXSTM_GetStat(file->stm) == 1) {
        file->rdsct = ADXSTM_Tell(file->stm) - file->skpos;
        adxf_CloseSjStm(file);
        file->state = 1;
        file->stop_requested = 0;
    }
}

static inline s32 adxf_Stop(ADXFFile* file)
{
    adxf_SetCmdHstry(5, 0, file, -1, -1);

    if (file == 0) {
        ADXERR_CallErrFunc1("E9040822:'adxf' is NULL.(ADXF_Stop)");
        return -1;
    }

    if (file->state == 1) {
        return file->skpos;
    }
    if (file->state == 3) {
        file->state = 1;
        return file->skpos;
    }
    if (file->stm == 0) {
        ADXERR_CallErrFunc1("E9040823:'adxf->stm' is NULL.(ADXF_Stop)");
        return -1;
    }

    ADXSTM_Stop(file->stm);
    ADXCRS_Lock();
    file->rdsct = ADXSTM_Tell(file->stm) - file->skpos;
    adxf_CloseSjStm(file);
    file->state = 1;
    ADXCRS_Unlock();
    adxf_SetCmdHstry(5, 1, file, -1, -1);
    return file->skpos;
}

void ADXF_Close(ADXFFile* file);

void ADXF_ExecServer(void)
{
    s32 index;

    ADXCRS_Lock();
    for (index = 0; index < 16; index++) {
        if (adxf_obj[index].used == 1) {
            adxf_ExecOne(&adxf_obj[index]);
        }
    }
    ADXCRS_Unlock();
}

void ADXF_CloseAll(void)
{
    s32 index;

    for (index = 0; index < 16; index++) {
        if (adxf_obj[index].used == 1) {
            ADXF_Close(&adxf_obj[index]);
        }
    }
}

void ADXF_Close(ADXFFile* file)
{
    ADXStream* stream;

    adxf_SetCmdHstry(3, 0, file, -1, -1);
    if (file != 0) {
        if (file->state == 2) {
            adxf_Stop(file);
        }

        if (file->stm != 0) {
            file->used = 0;
            stream = file->stm;
            file->stm = 0;
            ADXSTM_ReleaseFile(stream);
            ADXSTM_Destroy(stream);
        }

        memset(file, 0, sizeof(*file));
        adxf_SetCmdHstry(3, 1, file, -1, -1);
    }
}
