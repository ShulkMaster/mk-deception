#include "dolphin/card.h"
#include "runtime/cstring.h"

#include "__card.h"

static void DeleteCallback(s32 chan, s32 result)
{
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];
    callback = card->apiCallback;
    card->apiCallback = NULL;

    if (result >= 0) {
        result = __CARDFreeBlock(chan, card->startBlock, callback);
        if (result >= 0) {
            return;
        }
    }

    __CARDPutControlBlock(card, result);
    if (callback != NULL) {
        callback(chan, result);
    }
}

s32 CARDDeleteAsync(s32 chan, const char* fileName, CARDCallback callback)
{
    CARDControl* card;
    s32 fileNo;
    s32 result;
    CARDDir* dir;

    result = __CARDGetControlBlock(chan, &card);
    if (result < 0) {
        return result;
    }

    result = __CARDGetFileNo(card, fileName, &fileNo);
    if (result < 0) {
        return __CARDPutControlBlock(card, result);
    }
    if (__CARDIsOpened(card, fileNo)) {
        return __CARDPutControlBlock(card, CARD_RESULT_BUSY);
    }

    dir = &__CARDGetDirBlock(card)[fileNo];
    card->startBlock = dir->startBlock;
    memset(dir, 0xFF, sizeof(*dir));

    card->apiCallback = callback != NULL ? callback : __CARDDefaultApiCallback;
    result = __CARDUpdateDir(chan, DeleteCallback);
    if (result < 0) {
        __CARDPutControlBlock(card, result);
    }
    return result;
}

s32 CARDDelete(s32 chan, const char* fileName)
{
    s32 result;

    result = CARDDeleteAsync(chan, fileName, __CARDSyncCallback);
    if (result < 0) {
        return result;
    }
    return __CARDSync(chan);
}
