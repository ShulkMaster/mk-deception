#ifndef MKD_CRI_ADXF_INTERNAL_H
#define MKD_CRI_ADXF_INTERNAL_H

#include "cri/sj.h"
#include "dolphin/types.h"

typedef struct ADXStream ADXStream;

typedef struct ADXFCommandRecord {
    u8 command;
    u8 phase;
    u16 sequence;
    struct ADXFFile* file;
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
    s32 ptid;
    s32 flid;
    s32 skpos;
    s32 fnsct;
    s32 rqsct;
    s32 rdsct;
    void* buf;
    s32 bsize;
    u8 reserved_2C[0x18];
} ADXFFile;

typedef char ADXFCommandRecordSizeCheck[
    sizeof(ADXFCommandRecord) == 0x10 ? 1 : -1];
typedef char ADXFFileSizeCheck[sizeof(ADXFFile) == 0x44 ? 1 : -1];

void ADXF_CloseAll(void);

#endif
