#include "dolphin/card.h"
#include "dolphin/cache.h"

#include "__card.h"

#define TRUNC(n, a) (((u32)(n)) & ~((a) - 1))

static void ReadCallback(s32 chan, s32 result);

s32 __CARDSeek(CARDFileInfo* fileInfo, s32 length, s32 offset,
               CARDControl** cardOut)
{
    CARDControl* card;
    CARDDir* dir;
    CARDDir* entry;
    s32 result;
    u16* fat;

    result = __CARDGetControlBlock(fileInfo->chan, &card);
    if (result < 0) {
        return result;
    }
    if (!CARDIsValidBlockNo(card, fileInfo->iBlock) ||
        card->cBlock * card->sectorSize <= fileInfo->offset) {
        return __CARDPutControlBlock(card, CARD_RESULT_FATAL_ERROR);
    }

    dir = __CARDGetDirBlock(card);
    entry = &dir[fileInfo->fileNo];
    if (entry->length * card->sectorSize <= offset ||
        entry->length * card->sectorSize < offset + length) {
        return __CARDPutControlBlock(card, CARD_RESULT_LIMIT);
    }

    card->fileInfo = fileInfo;
    fileInfo->length = length;
    if (offset < fileInfo->offset) {
        fileInfo->offset = 0;
        fileInfo->iBlock = entry->startBlock;
        if (!CARDIsValidBlockNo(card, fileInfo->iBlock)) {
            return __CARDPutControlBlock(card, CARD_RESULT_BROKEN);
        }
    }

    fat = __CARDGetFatBlock(card);
    while (fileInfo->offset < TRUNC(offset, card->sectorSize)) {
        fileInfo->offset += card->sectorSize;
        fileInfo->iBlock = fat[fileInfo->iBlock];
        if (!CARDIsValidBlockNo(card, fileInfo->iBlock)) {
            return __CARDPutControlBlock(card, CARD_RESULT_BROKEN);
        }
    }

    fileInfo->offset = offset;
    *cardOut = card;
    return CARD_RESULT_READY;
}

static void ReadCallback(s32 chan, s32 result)
{
    CARDControl* card = &__CARDBlock[chan];
    CARDFileInfo* fileInfo;
    CARDCallback callback;
    u16* fat;
    s32 length;

    if (result >= 0) {
        fileInfo = card->fileInfo;
        if (fileInfo->length < 0) {
            result = CARD_RESULT_CANCELED;
        } else {
            length = TRUNC(fileInfo->offset + card->sectorSize,
                           card->sectorSize) - fileInfo->offset;
            fileInfo->length -= length;
            if (fileInfo->length > 0) {
                fat = __CARDGetFatBlock(card);
                fileInfo->offset += length;
                fileInfo->iBlock = fat[fileInfo->iBlock];
                if (!CARDIsValidBlockNo(card, fileInfo->iBlock)) {
                    result = CARD_RESULT_BROKEN;
                } else {
                    length = fileInfo->length < card->sectorSize
                                 ? fileInfo->length
                                 : card->sectorSize;
                    result = __CARDRead(chan,
                                        card->sectorSize *
                                            (u32)fileInfo->iBlock,
                                        length, card->buffer, ReadCallback);
                    if (result >= 0) {
                        return;
                    }
                }
            }
        }
    }

    callback = card->apiCallback;
    card->apiCallback = NULL;
    __CARDPutControlBlock(card, result);
    callback(chan, result);
}

s32 CARDReadAsync(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset,
                  CARDCallback callback)
{
    CARDControl* card;
    CARDDir* entry;
    s32 result;

    if ((offset & (CARD_SEG_SIZE - 1)) != 0 ||
        (length & (CARD_SEG_SIZE - 1)) != 0) {
        return CARD_RESULT_FATAL_ERROR;
    }
    result = __CARDSeek(fileInfo, length, offset, &card);
    if (result < 0) {
        return result;
    }

    entry = &__CARDGetDirBlock(card)[fileInfo->fileNo];
    result = __CARDIsReadable(card, entry);
    if (result < 0) {
        return __CARDPutControlBlock(card, result);
    }

    DCInvalidateRange(buffer, (u32)length);
    card->apiCallback = callback ? callback : __CARDDefaultApiCallback;
    offset = fileInfo->offset & (card->sectorSize - 1);
    length = length < card->sectorSize - offset
                 ? length
                 : card->sectorSize - offset;
    result = __CARDRead(fileInfo->chan,
                        card->sectorSize * (u32)fileInfo->iBlock + offset,
                        length, buffer, ReadCallback);
    if (result < 0) {
        __CARDPutControlBlock(card, result);
    }
    return result;
}

s32 CARDRead(CARDFileInfo* fileInfo, void* buffer, s32 length, s32 offset)
{
    s32 result = CARDReadAsync(fileInfo, buffer, length, offset,
                               __CARDSyncCallback);
    if (result < 0) {
        return result;
    }
    return __CARDSync(fileInfo->chan);
}
