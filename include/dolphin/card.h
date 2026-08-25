#ifndef DOLPHIN_CARD_H
#define DOLPHIN_CARD_H

#include "dolphin/os.h"
#include "dolphin/dvd.h"
#include "dolphin/dsp.h"

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed long s32;
typedef unsigned long u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef int BOOL;

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define CARD_FILENAME_MAX 32
#define CARD_MAX_FILE 127
#define CARD_ICON_MAX 8
#define CARD_NUM_SYSTEM_BLOCK 5
#define CARD_SYSTEM_BLOCK_SIZE (8 * 1024u)
#define CARD_SEG_SIZE 0x200u
#define CARD_MAX_MOUNT_STEP (CARD_NUM_SYSTEM_BLOCK + 2)
#define CARD_FAT_FREEBLOCKS 3
#define CARD_FAT_AVAIL 0x0000u
#define CARD_FAT_CHECKSUM 0x0000u
#define CARD_FAT_CHECKSUMINV 0x0001u
#define CARD_FAT_CHECKCODE 0x0002u
#define CARD_FAT_LASTSLOT 0x0004u
#define CARD_WORKAREA_SIZE (5 * CARD_SYSTEM_BLOCK_SIZE)
#define CARD_ENCODE_ANSI 0
#define CARD_ENCODE_SJIS 1

#define CARDIsValidBlockNo(card, blockNo) \
    ((u16)(CARD_NUM_SYSTEM_BLOCK) <= (u16)(blockNo) && \
     (u16)(blockNo) < (u16)((card)->cBlock))

#define CARD_RESULT_READY 0
#define CARD_RESULT_UNLOCKED 1
#define CARD_RESULT_BUSY -1
#define CARD_RESULT_WRONGDEVICE -2
#define CARD_RESULT_NOCARD -3
#define CARD_RESULT_NOFILE -4
#define CARD_RESULT_IOERROR -5
#define CARD_RESULT_BROKEN -6
#define CARD_RESULT_EXIST -7
#define CARD_RESULT_NOENT -8
#define CARD_RESULT_INSSPACE -9
#define CARD_RESULT_NOPERM -10
#define CARD_RESULT_LIMIT -11
#define CARD_RESULT_NAMETOOLONG -12
#define CARD_RESULT_ENCODING -13
#define CARD_RESULT_CANCELED -14
#define CARD_RESULT_FATAL_ERROR -128

typedef void (*CARDCallback)(s32 chan, s32 result);

typedef struct CARDFileInfo {
    s32 chan;
    s32 fileNo;
    s32 offset;
    s32 length;
    u16 iBlock;
    u16 padding;
} CARDFileInfo;

typedef struct CARDDir {
    u8 gameName[4];
    u8 company[2];
    u8 reserved_06;
    u8 bannerFormat;
    u8 fileName[CARD_FILENAME_MAX];
    u32 time;
    u32 iconAddr;
    u16 iconFormat;
    u16 iconSpeed;
    u8 permission;
    u8 copyTimes;
    u16 startBlock;
    u16 length;
    u8 reserved_3A[2];
    u32 commentAddr;
} CARDDir;

typedef struct CARDStat {
    char fileName[CARD_FILENAME_MAX];
    u32 length;
    u32 time;
    u8 gameName[4];
    u8 company[2];
    u8 bannerFormat;
    u8 reserved_2F;
    u32 iconAddr;
    u16 iconFormat;
    u16 iconSpeed;
    u32 commentAddr;
    u32 offsetBanner;
    u32 offsetBannerTlut;
    u32 offsetIcon[CARD_ICON_MAX];
    u32 offsetIconTlut;
    u32 offsetData;
} CARDStat;

typedef struct CARDDirCheck {
    u8 reserved_00[56];
    u16 reserved_38;
    s16 checkCode;
    u16 checkSum;
    u16 checkSumInv;
} CARDDirCheck;

typedef struct CARDID {
    u8 serial[32];
    u16 deviceID;
    u16 size;
    u16 encode;
    u8 padding[470];
    u16 checkSum;
    u16 checkSumInv;
} CARDID;

#define CARDGetDirCheck(dir) ((CARDDirCheck*)&(dir)[CARD_MAX_FILE])

typedef struct CARDControl {
    BOOL attached;
    s32 result;
    u16 size;
    u16 pageSize;
    s32 sectorSize;
    u16 cBlock;
    u16 vendorID;
    s32 latency;
    u8 id[12];
    s32 mountStep;
    s32 formatStep;
    u32 scramble;
    DSPTaskInfo task;
    void* workArea;
    CARDDir* currentDir;
    u16* currentFat;
    OSThreadQueue threadQueue;
    u8 cmd[9];
    u8 reserved_9D[3];
    s32 cmdlen;
    volatile u32 mode;
    s32 retry;
    s32 repeat;
    u32 addr;
    void* buffer;
    s32 xferred;
    u16 freeNo;
    u16 startBlock;
    CARDFileInfo* fileInfo;
    CARDCallback extCallback;
    CARDCallback txCallback;
    CARDCallback exiCallback;
    CARDCallback apiCallback;
    CARDCallback xferCallback;
    CARDCallback eraseCallback;
    CARDCallback unlockCallback;
    OSAlarm alarm;
    u32 cid;
    const DVDDiskID* diskID;
} CARDControl;

typedef struct CARDDecParam {
    u8* inputAddr;
    u32 inputLength;
    u32 aramAddr;
    u8* outputAddr;
} CARDDecParam;

typedef char CARDFileInfoSizeCheck[sizeof(CARDFileInfo) == 0x14 ? 1 : -1];
typedef char CARDDirSizeCheck[sizeof(CARDDir) == 0x40 ? 1 : -1];
typedef char CARDStatSizeCheck[sizeof(CARDStat) == 0x6C ? 1 : -1];
typedef char CARDDirCheckSizeCheck[sizeof(CARDDirCheck) == 0x40 ? 1 : -1];
typedef char CARDIDSizeCheck[sizeof(CARDID) == 0x200 ? 1 : -1];
typedef char CARDControlSizeCheck[sizeof(CARDControl) == 0x110 ? 1 : -1];
typedef char CARDDecParamSizeCheck[sizeof(CARDDecParam) == 0x10 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

s32 CARDDeleteAsync(s32 chan, const char* fileName, CARDCallback callback);
s32 CARDDelete(s32 chan, const char* fileName);
s32 CARDReadAsync(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset,
                  CARDCallback callback);
s32 CARDRead(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset);
s32 CARDWriteAsync(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset,
                   CARDCallback callback);
s32 CARDWrite(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset);
s32 CARDGetSerialNo(s32 chan, u64* serialNo);
s32 CARDSetAttributesAsync(s32 chan, s32 fileNo, u8 attributes,
                           CARDCallback callback);
s32 CARDSetAttributes(s32 chan, s32 fileNo, u8 attributes);
s32 CARDCreateAsync(s32 chan, const char* fileName, u32 size,
                    CARDFileInfo* fileInfo, CARDCallback callback);
s32 CARDCreate(s32 chan, const char* fileName, u32 size,
               CARDFileInfo* fileInfo);
s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat* stat);
s32 CARDSetStatusAsync(s32 chan, s32 fileNo, CARDStat* stat,
                       CARDCallback callback);
s32 CARDSetStatus(s32 chan, s32 fileNo, CARDStat* stat);
s32 CARDCheckExAsync(s32 chan, s32* xferBytes, CARDCallback callback);
s32 CARDCheck(s32 chan);
s32 CARDFormat(s32 chan);
s32 CARDProbeEx(s32 chan, s32* memSize, s32* sectorSize);
s32 CARDMountAsync(s32 chan, void* workArea, CARDCallback detachCallback,
                   CARDCallback attachCallback);
s32 CARDMount(s32 chan, void* workArea, CARDCallback detachCallback);
void CARDInit(void);
s32 CARDUnmount(s32 chan);
BOOL CARDGetFastMode(void);
s32 CARDOpen(s32 chan, const char* fileName, CARDFileInfo* fileInfo);
s32 CARDClose(CARDFileInfo* fileInfo);

#ifdef __cplusplus
}
#endif

#endif
