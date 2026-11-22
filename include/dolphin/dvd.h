#ifndef DOLPHIN_DVD_H
#define DOLPHIN_DVD_H

#include "dolphin/types.h"
#include "platform/os_types.h"

struct OSContext;
struct OSAlarm;

typedef struct DVDDiskID {
    char gameName[4];
    char company[2];
    unsigned char diskNumber;
    unsigned char gameVersion;
    unsigned char streaming;
    unsigned char streamingBufSize;
    unsigned char padding[22];
} DVDDiskID;

typedef struct DVDCommandBlock DVDCommandBlock;
typedef struct DVDFileInfo DVDFileInfo;
typedef struct DVDDriveInfo {
    unsigned short revision_level;
    unsigned short device_code;
    u32 release_date;
    unsigned char padding[24];
} DVDDriveInfo;
typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock* block);
typedef void (*DVDCallback)(s32 result, DVDFileInfo* file_info);
typedef void (*DVDLowCallback)(u32 interrupt_type);
typedef void (*DVDCommandCheckerCallback)(u32 result);
typedef void (*DVDCommandChecker)(DVDCommandBlock* block,
                                  DVDCommandCheckerCallback callback);

#define DVD_STATE_FATAL_ERROR -1
#define DVD_STATE_END 0
#define DVD_STATE_BUSY 1
#define DVD_STATE_WAITING 2
#define DVD_STATE_COVER_CLOSED 3
#define DVD_STATE_NO_DISK 4
#define DVD_STATE_COVER_OPEN 5
#define DVD_STATE_WRONG_DISK 6
#define DVD_STATE_MOTOR_STOPPED 7
#define DVD_STATE_PAUSING 8
#define DVD_STATE_IGNORED 9
#define DVD_STATE_CANCELED 10
#define DVD_STATE_RETRY 11
#define DVD_INTTYPE_TC 1
#define DVD_INTTYPE_DE 2
#define DVD_INTTYPE_CVR 4
#define DVD_COMMAND_NONE 0
#define DVD_COMMAND_READ 1
#define DVD_COMMAND_SEEK 2
#define DVD_COMMAND_CHANGE_DISK 3
#define DVD_COMMAND_BSREAD 4
#define DVD_COMMAND_READID 5
#define DVD_COMMAND_INITSTREAM 6
#define DVD_COMMAND_CANCELSTREAM 7
#define DVD_COMMAND_STOP_STREAM_AT_END 8
#define DVD_COMMAND_REQUEST_AUDIO_ERROR 9
#define DVD_COMMAND_REQUEST_PLAY_ADDR 10
#define DVD_COMMAND_REQUEST_START_ADDR 11
#define DVD_COMMAND_REQUEST_LENGTH 12
#define DVD_COMMAND_AUDIO_BUFFER_CONFIG 13
#define DVD_COMMAND_INQUIRY 14
#define DVD_COMMAND_BS_CHANGE_DISK 15
#define DVD_COMMAND_UNK_16 16

struct DVDCommandBlock {
    DVDCommandBlock* next;
    DVDCommandBlock* prev;
    u32 command;
    s32 state;
    u32 offset;
    u32 length;
    void* address;
    u32 current_transfer_size;
    u32 transferred_size;
    DVDDiskID* id;
    DVDCBCallback callback;
    void* user_data;
};

struct DVDFileInfo {
    DVDCommandBlock cb;
    u32 start_address;
    u32 length;
    DVDCallback callback;
};
typedef char DVDDiskIDSizeCheck[sizeof(DVDDiskID) == 0x20 ? 1 : -1];
typedef char DVDCommandBlockSizeCheck[sizeof(DVDCommandBlock) == 0x30 ? 1 : -1];
typedef char DVDFileInfoSizeCheck[sizeof(DVDFileInfo) == 0x3C ? 1 : -1];

typedef struct DVDBB2 {
    u32 boot_file_position;
    u32 fst_position;
    u32 fst_length;
    u32 fst_max_length;
    void* fst_address;
    u32 user_position;
    u32 user_length;
    u32 padding0;
} DVDBB2;
typedef char DVDBB2SizeCheck[sizeof(DVDBB2) == 0x20 ? 1 : -1];
typedef char DVDDriveInfoSizeCheck[sizeof(DVDDriveInfo) == 0x20 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

int DVDCheckDisk(void);
s32 DVDGetDriveStatus(void);
void DVDInit(void);
DVDCommandChecker __DVDSetOptionalCommandChecker(DVDCommandChecker checker);
int DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info,
                    DVDCBCallback callback);
void __DVDPrintFatalMessage(void);
int DVDCompareDiskID(const DVDDiskID* first, const DVDDiskID* second);
void __DVDStoreErrorCode(u32 error);
void __DVDClearWaitingQueue(void);
int __DVDPushWaitingQueue(int priority, DVDCommandBlock* block);
DVDCommandBlock* __DVDPopWaitingQueue(void);
int __DVDCheckWaitingQueue(void);
int __DVDDequeueWaitingQueue(DVDCommandBlock* block);
int __DVDIsBlockInWaitingQueue(DVDCommandBlock* block);
int DVDReadAbsAsyncForBS(DVDCommandBlock* block, void* address,
                         s32 length, s32 offset,
                         DVDCBCallback callback);
int DVDReadDiskID(DVDCommandBlock* block, DVDDiskID* id,
                  DVDCBCallback callback);
int DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* address, s32 length,
                        s32 offset, DVDCBCallback callback, s32 priority);
s32 DVDCancel(volatile DVDCommandBlock* block);
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block);
int DVDSetAutoInvalidation(int enabled);
void DVDResume(void);
DVDDiskID* DVDGetCurrentDiskID(void);
int DVDCancelStreamAsync(DVDCommandBlock* block, DVDCBCallback callback);
void __DVDPrepareResetAsync(DVDCBCallback callback);
void __DVDInitWA(void);
void __DVDInterruptHandler(short interrupt, struct OSContext* context);
int DVDLowRead(void* address, u32 length, u32 offset,
               DVDLowCallback callback);
int DVDLowSeek(u32 offset, DVDLowCallback callback);
int DVDLowWaitCoverClose(DVDLowCallback callback);
int DVDLowReadDiskID(DVDDiskID* disk_id, DVDLowCallback callback);
int DVDLowStopMotor(DVDLowCallback callback);
int DVDLowRequestError(DVDLowCallback callback);
int DVDLowInquiry(DVDDriveInfo* info, DVDLowCallback callback);
int DVDLowAudioStream(u32 subcommand, u32 length,
                      u32 offset, DVDLowCallback callback);
int DVDLowRequestAudioStatus(u32 subcommand, DVDLowCallback callback);
int DVDLowAudioBufferConfig(int enable, u32 size,
                            DVDLowCallback callback);
void DVDLowReset(void);
int DVDLowBreak(void);
DVDLowCallback DVDLowClearCallback(void);
void __DVDLowSetWAType(u32 type, s32 seek_location);
int __DVDLowTestAlarm(const struct OSAlarm* alarm);
int __DVDTestAlarm(const struct OSAlarm* alarm);
void __DVDFSInit(void);
s32 DVDConvertPathToEntrynum(const char* path);
int DVDFastOpen(s32 entry_number, DVDFileInfo* file_info);
int DVDOpen(const char* file_name, DVDFileInfo* file_info);
int DVDClose(DVDFileInfo* file_info);
int DVDGetCurrentDir(char* path, u32 max_length);
int DVDReadAsyncPrio(DVDFileInfo* file_info, void* address, s32 length,
                     s32 offset, DVDCallback callback, s32 priority);
s32 DVDReadPrio(DVDFileInfo* file_info, void* address, s32 length,
                s32 offset, s32 priority);
s32 DVDGetTransferredSize(DVDFileInfo* file_info);
void DVDReset(void);
void __fstLoad(void);
extern OSThreadQueue __DVDThreadQueue;

#ifdef __cplusplus
}
#endif

#endif
