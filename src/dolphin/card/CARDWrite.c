#include "dolphin/card.h"
#include "dolphin/cache.h"

#include "__card.h"

static void WriteCallback(s32 chan, s32 result);
static void EraseCallback(s32 chan, s32 result);

static void WriteCallback(s32 chan, s32 result)
{
    CARDControl* card = &__CARDBlock[chan];
    CARDFileInfo* fileInfo;
    CARDCallback callback;
    CARDDir* entry;
    u16* fat;

    if (result >= 0) {
        fileInfo = card->fileInfo;
        if (fileInfo->length < 0) {
            result = CARD_RESULT_CANCELED;
        } else {
            fileInfo->length -= card->sectorSize;
            if (fileInfo->length <= 0) {
                entry = &__CARDGetDirBlock(card)[fileInfo->fileNo];
                entry->time = OSGetTime() / (__OSBusClock / 4);
                callback = card->apiCallback;
                card->apiCallback = NULL;
                result = __CARDUpdateDir(chan, callback);
            } else {
                fat = __CARDGetFatBlock(card);
                fileInfo->offset += card->sectorSize;
                fileInfo->iBlock = fat[fileInfo->iBlock];
                if (!CARDIsValidBlockNo(card, fileInfo->iBlock)) {
                    result = CARD_RESULT_BROKEN;
                } else {
                    result = __CARDEraseSector(
                        chan, card->sectorSize * fileInfo->iBlock,
                        EraseCallback);
                }
            }
        }
        if (result >= 0) {
            return;
        }
    }

    callback = card->apiCallback;
    card->apiCallback = NULL;
    __CARDPutControlBlock(card, result);
    callback(chan, result);
}

static void EraseCallback(s32 chan, s32 result)
{
    CARDControl* card = &__CARDBlock[chan];
    CARDCallback callback;
    CARDFileInfo* fileInfo;

    if (result >= 0) {
        fileInfo = card->fileInfo;
        result = __CARDWrite(chan, card->sectorSize * fileInfo->iBlock,
                             card->sectorSize, card->buffer, WriteCallback);
        if (result >= 0) {
            return;
        }
    }

    callback = card->apiCallback;
    card->apiCallback = NULL;
    __CARDPutControlBlock(card, result);
    callback(chan, result);
}

s32 CARDWriteAsync(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset,
                   CARDCallback callback)
{
    CARDControl* card;
    CARDDir* entry;
    s32 result;

    result = __CARDSeek(fileInfo, length, offset, &card);
    if (result < 0) {
        return result;
    }
    if ((offset & (card->sectorSize - 1)) != 0 ||
        (length & (card->sectorSize - 1)) != 0) {
        return __CARDPutControlBlock(card, CARD_RESULT_FATAL_ERROR);
    }

    entry = &__CARDGetDirBlock(card)[fileInfo->fileNo];
    result = __CARDIsWritable(card, entry);
    if (result < 0) {
        return __CARDPutControlBlock(card, result);
    }

    DCStoreRange(buffer, (u32)length);
    card->apiCallback = callback ? callback : __CARDDefaultApiCallback;
    card->buffer = buffer;
    result = __CARDEraseSector(
        fileInfo->chan, card->sectorSize * (u32)fileInfo->iBlock,
        EraseCallback);
    if (result < 0) {
        __CARDPutControlBlock(card, result);
    }
    return result;
}

s32 CARDWrite(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset)
{
    s32 result = CARDWriteAsync(fileInfo, buffer, length, offset,
                                __CARDSyncCallback);
    if (result < 0) {
        return result;
    }
    return __CARDSync(fileInfo->chan);
}
