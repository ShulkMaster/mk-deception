#ifndef DOLPHIN_CARD_INTERNAL_H
#define DOLPHIN_CARD_INTERNAL_H

#include "dolphin/card.h"

extern CARDControl __CARDBlock[2];
extern DVDDiskID __CARDDiskNone;
extern u8 __CARDPermMask;
extern u16 __CARDVendorID;
#define __gUnknown800030E3 (*(volatile u8*)0x800030E3)

void __CARDDefaultApiCallback(s32 chan, s32 result);
void __CARDSyncCallback(s32 chan, s32 result);
void __CARDExtHandler(s32 chan, OSContext* context);
void __CARDExiHandler(s32 chan, OSContext* context);
void __CARDTxHandler(s32 chan, OSContext* context);
void __CARDUnlockedHandler(s32 chan, OSContext* context);
s32 __CARDEnableInterrupt(s32 chan, BOOL enable);
s32 __CARDReadStatus(s32 chan, u8* status);
int __CARDReadVendorID(s32 chan, u16* vendorID);
s32 __CARDClearStatus(s32 chan);
s32 __CARDGetControlBlock(s32 chan, CARDControl** card);
s32 __CARDPutControlBlock(CARDControl* card, s32 result);
s32 __CARDSync(s32 chan);

CARDDir* __CARDGetDirBlock(CARDControl* card);
s32 __CARDUpdateDir(s32 chan, CARDCallback callback);
s32 __CARDWrite(s32 chan, u32 address, s32 length, void* source,
                CARDCallback callback);
s32 __CARDRead(s32 chan, u32 address, s32 length, void* destination,
               CARDCallback callback);
s32 __CARDSeek(CARDFileInfo* fileInfo, s32 length, s32 offset,
               CARDControl** card);
s32 __CARDEraseSector(s32 chan, u32 address, CARDCallback callback);
s32 __CARDReadSegment(s32 chan, CARDCallback callback);
s32 __CARDWritePage(s32 chan, CARDCallback callback);
u16 __CARDGetFontEncode(void);
void __CARDSetDiskID(const DVDDiskID* diskID);
s32 __CARDUnlock(s32 chan, u8* id);
s32 __CARDFormatRegionAsync(s32 chan, u16 encode, CARDCallback callback);
void __CARDCheckSum(void* data, int length, u16* checksum,
                    u16* checksumInverse);
s32 __CARDVerify(CARDControl* card);
void __CARDMountCallback(s32 chan, s32 result);
s32 __CARDFreeBlock(s32 chan, u16 startBlock, CARDCallback callback);
void* __CARDGetFatBlock(CARDControl* card);
s32 __CARDAllocBlock(s32 chan, u32 size, CARDCallback callback);
s32 __CARDUpdateFatBlock(s32 chan, u16* fat, CARDCallback callback);
s32 __CARDGetFileNo(CARDControl* card, const char* fileName, s32* fileNo);
BOOL __CARDCompareFileName(CARDDir* dir, const char* fileName);
s32 __CARDAccess(CARDControl* card, CARDDir* dir);
s32 __CARDIsReadable(CARDControl* card, CARDDir* dir);
s32 __CARDIsWritable(CARDControl* card, CARDDir* dir);
BOOL __CARDIsOpened(CARDControl* card, s32 fileNo);
s32 __CARDGetStatusEx(s32 chan, s32 fileNo, CARDDir* status);
s32 __CARDSetStatusExAsync(s32 chan, s32 fileNo, CARDDir* status,
                           CARDCallback callback);

#endif
