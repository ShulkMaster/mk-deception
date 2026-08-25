#ifndef DOLPHIN_DVD_H
#define DOLPHIN_DVD_H

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
    unsigned long release_date;
    unsigned char padding[24];
} DVDDriveInfo;
typedef void (*DVDCBCallback)(long result, DVDCommandBlock* block);
typedef void (*DVDCallback)(long result, DVDFileInfo* file_info);
typedef void (*DVDLowCallback)(unsigned long interrupt_type);
typedef void (*DVDCommandCheckerCallback)(unsigned long result);
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
    unsigned long command;
    long state;
    unsigned long offset;
    unsigned long length;
    void* address;
    unsigned long current_transfer_size;
    unsigned long transferred_size;
    DVDDiskID* id;
    DVDCBCallback callback;
    void* user_data;
};

struct DVDFileInfo {
    DVDCommandBlock cb;
    unsigned long start_address;
    unsigned long length;
    DVDCallback callback;
};
typedef char DVDDiskIDSizeCheck[sizeof(DVDDiskID) == 0x20 ? 1 : -1];
typedef char DVDCommandBlockSizeCheck[sizeof(DVDCommandBlock) == 0x30 ? 1 : -1];
typedef char DVDFileInfoSizeCheck[sizeof(DVDFileInfo) == 0x3C ? 1 : -1];

typedef struct DVDBB2 {
    unsigned long boot_file_position;
    unsigned long fst_position;
    unsigned long fst_length;
    unsigned long fst_max_length;
    void* fst_address;
    unsigned long user_position;
    unsigned long user_length;
    unsigned long padding;
} DVDBB2;
typedef char DVDBB2SizeCheck[sizeof(DVDBB2) == 0x20 ? 1 : -1];
typedef char DVDDriveInfoSizeCheck[sizeof(DVDDriveInfo) == 0x20 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

int DVDCheckDisk(void);
signed long DVDGetDriveStatus(void);
void DVDInit(void);
DVDCommandChecker __DVDSetOptionalCommandChecker(DVDCommandChecker checker);
int DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info,
                    DVDCBCallback callback);
void __DVDPrintFatalMessage(void);
int DVDCompareDiskID(const DVDDiskID* first, const DVDDiskID* second);
void __DVDStoreErrorCode(unsigned long error);
void __DVDClearWaitingQueue(void);
int __DVDPushWaitingQueue(int priority, DVDCommandBlock* block);
DVDCommandBlock* __DVDPopWaitingQueue(void);
int __DVDCheckWaitingQueue(void);
int __DVDDequeueWaitingQueue(DVDCommandBlock* block);
int __DVDIsBlockInWaitingQueue(DVDCommandBlock* block);
int DVDReadAbsAsyncForBS(DVDCommandBlock* block, void* address,
                         signed long length, signed long offset,
                         DVDCBCallback callback);
int DVDReadDiskID(DVDCommandBlock* block, DVDDiskID* id,
                  DVDCBCallback callback);
int DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* address, long length,
                        long offset, DVDCBCallback callback, long priority);
signed long DVDCancel(volatile DVDCommandBlock* block);
long DVDGetCommandBlockStatus(const DVDCommandBlock* block);
int DVDSetAutoInvalidation(int enabled);
void DVDResume(void);
DVDDiskID* DVDGetCurrentDiskID(void);
int DVDCancelStreamAsync(DVDCommandBlock* block, DVDCBCallback callback);
void __DVDPrepareResetAsync(DVDCBCallback callback);
void __DVDInitWA(void);
void __DVDInterruptHandler(short interrupt, struct OSContext* context);
int DVDLowRead(void* address, unsigned long length, unsigned long offset,
               DVDLowCallback callback);
int DVDLowSeek(unsigned long offset, DVDLowCallback callback);
int DVDLowWaitCoverClose(DVDLowCallback callback);
int DVDLowReadDiskID(DVDDiskID* disk_id, DVDLowCallback callback);
int DVDLowStopMotor(DVDLowCallback callback);
int DVDLowRequestError(DVDLowCallback callback);
int DVDLowInquiry(DVDDriveInfo* info, DVDLowCallback callback);
int DVDLowAudioStream(unsigned long subcommand, unsigned long length,
                      unsigned long offset, DVDLowCallback callback);
int DVDLowRequestAudioStatus(unsigned long subcommand, DVDLowCallback callback);
int DVDLowAudioBufferConfig(int enable, unsigned long size,
                            DVDLowCallback callback);
void DVDLowReset(void);
int DVDLowBreak(void);
DVDLowCallback DVDLowClearCallback(void);
void __DVDLowSetWAType(unsigned long type, signed long seek_location);
int __DVDLowTestAlarm(const struct OSAlarm* alarm);
int __DVDTestAlarm(const struct OSAlarm* alarm);
void __DVDFSInit(void);
long DVDConvertPathToEntrynum(const char* path);
int DVDFastOpen(long entry_number, DVDFileInfo* file_info);
int DVDOpen(const char* file_name, DVDFileInfo* file_info);
int DVDClose(DVDFileInfo* file_info);
int DVDGetCurrentDir(char* path, unsigned long max_length);
int DVDReadAsyncPrio(DVDFileInfo* file_info, void* address, long length,
                     long offset, DVDCallback callback, long priority);
long DVDReadPrio(DVDFileInfo* file_info, void* address, long length,
                 long offset, long priority);
long DVDGetTransferredSize(DVDFileInfo* file_info);
void DVDReset(void);
void __fstLoad(void);
extern struct OSThreadQueue __DVDThreadQueue;

#ifdef __cplusplus
}
#endif

#endif
