#ifndef DOLPHIN_CARD_H
#define DOLPHIN_CARD_H

typedef struct CARDFileInfo {
    long chan;
    long fileNo;
    long offset;
    long length;
    unsigned short iBlock;
    unsigned short padding;
    unsigned char pad14[0x20 - 0x14];
} CARDFileInfo;

typedef struct CARDStat {
    char fileName[32];
    unsigned long length;
    unsigned long time;
    char gameName[4];
    char company[2];
    unsigned char bannerFormat;
    unsigned char padding;
    unsigned long iconAddr;
    unsigned short iconFormat;
    unsigned short iconSpeed;
    unsigned long commentAddr;
    unsigned long offsetBanner;
    unsigned long offsetBannerTlut;
    unsigned long offsetIcon[8];
    unsigned long offsetIconTlut;
    unsigned long offsetData;
} CARDStat;

#ifdef __cplusplus
extern "C" {
#endif

long CARDInit(void);
long CARDProbeEx(long chan, long* mem_size, long* sector_size);
long CARDMount(long chan, void* work_area, void (*detach)(long, long));
long CARDCheck(long chan);
long CARDFreeBlocks(long chan, long* bytes_not_used, long* files_not_used);
long CARDOpen(long chan, const char* file_name, CARDFileInfo* file_info);
long CARDClose(CARDFileInfo* file_info);
long CARDUnmount(long chan);
long CARDFormat(long chan);
long CARDCreate(long chan, const char* file_name, unsigned long size,
                CARDFileInfo* file_info);
long CARDWrite(CARDFileInfo* file_info, const void* buffer, long length,
               long offset);
long CARDRead(CARDFileInfo* file_info, void* buffer, long length, long offset);
long CARDDelete(long chan, const char* file_name);
long CARDSetAttributes(long chan, long file_no, unsigned char attributes);
long CARDGetSerialNo(long chan, unsigned long long* serial_no);
long CARDGetStatus(long chan, long file_no, CARDStat* status);
long CARDSetStatus(long chan, long file_no, CARDStat* status);

#ifdef __cplusplus
}
#endif

#endif
