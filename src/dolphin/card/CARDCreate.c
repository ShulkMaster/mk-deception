#include "dolphin/card.h"
#include "runtime/cstring.h"

#include "__card.h"

static void CreateCallbackFat(s32 chan, s32 result)
{
    CARDControl* card;
    CARDDir* dir;
    CARDDir* entry;
    CARDCallback callback;

    card = &__CARDBlock[chan];
    callback = card->apiCallback;
    card->apiCallback = NULL;
    if (result >= 0) {
        dir = __CARDGetDirBlock(card);
        entry = &dir[card->freeNo];
        memcpy(entry->gameName, card->diskID->gameName,
               sizeof(entry->gameName));
        memcpy(entry->company, card->diskID->company,
               sizeof(entry->company));
        entry->permission = 4;
        entry->copyTimes = 0;
        entry->startBlock = card->startBlock;
        entry->bannerFormat = 0;
        entry->iconAddr = (u32)-1;
        entry->iconFormat = 0;
        entry->iconSpeed = 0;
        entry->commentAddr = (u32)-1;
        entry->iconSpeed = (entry->iconSpeed & ~3) | 1;
        card->fileInfo->offset = 0;
        card->fileInfo->iBlock = entry->startBlock;
        entry->time = OSGetTime() / (__OSBusClock / 4);
        result = __CARDUpdateDir(chan, callback);
        if (result >= 0) {
            return;
        }
    }

    __CARDPutControlBlock(card, result);
    if (callback != NULL) {
        callback(chan, result);
    }
}

s32 CARDCreateAsync(s32 chan, const char* fileName, u32 size,
                    CARDFileInfo* fileInfo, CARDCallback callback)
{
    CARDControl* card;
    CARDDir* dir;
    CARDDir* entry;
    u16 fileNo;
    u16 freeNo;
    u16* fat;
    s32 result;

    if (strlen(fileName) > CARD_FILENAME_MAX) {
        return CARD_RESULT_NAMETOOLONG;
    }
    result = __CARDGetControlBlock(chan, &card);
    if (result < 0) {
        return result;
    }
    if (size == 0 || size % card->sectorSize != 0) {
        return CARD_RESULT_FATAL_ERROR;
    }

    freeNo = (u16)-1;
    dir = __CARDGetDirBlock(card);
    for (fileNo = 0; fileNo < CARD_MAX_FILE; fileNo++) {
        entry = &dir[fileNo];
        if (entry->gameName[0] == 0xFF) {
            if (freeNo == (u16)-1) {
                freeNo = fileNo;
            }
        } else if (memcmp(entry->gameName, card->diskID->gameName,
                          sizeof(entry->gameName)) == 0 &&
                   memcmp(entry->company, card->diskID->company,
                          sizeof(entry->company)) == 0 &&
                   __CARDCompareFileName(entry, fileName)) {
            return __CARDPutControlBlock(card, CARD_RESULT_EXIST);
        }
    }

    if (freeNo == (u16)-1) {
        return __CARDPutControlBlock(card, CARD_RESULT_NOENT);
    }
    fat = __CARDGetFatBlock(card);
    if (card->sectorSize * fat[CARD_FAT_FREEBLOCKS] < size) {
        return __CARDPutControlBlock(card, CARD_RESULT_INSSPACE);
    }

    card->apiCallback = callback ? callback : __CARDDefaultApiCallback;
    card->freeNo = freeNo;
    entry = &dir[freeNo];
    entry->length = size / card->sectorSize;
    strncpy((char*)entry->fileName, fileName, CARD_FILENAME_MAX);
    card->fileInfo = fileInfo;
    fileInfo->chan = chan;
    fileInfo->fileNo = freeNo;

    result = __CARDAllocBlock(chan, size / card->sectorSize,
                              CreateCallbackFat);
    if (result < 0) {
        return __CARDPutControlBlock(card, result);
    }
    return result;
}

s32 CARDCreate(s32 chan, const char* fileName, u32 size,
               CARDFileInfo* fileInfo)
{
    s32 result = CARDCreateAsync(chan, fileName, size, fileInfo,
                                 __CARDSyncCallback);
    if (result < 0) {
        return result;
    }
    return __CARDSync(chan);
}
