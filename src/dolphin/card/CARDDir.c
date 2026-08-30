#include "dolphin/cache.h"
#include "dolphin/card.h"
#include "runtime/cstring.h"

#include "__card.h"

static void WriteCallback(s32 chan, s32 result);
static void EraseCallback(s32 chan, s32 result);

CARDDir* __CARDGetDirBlock(CARDControl* card)
{
    return card->currentDir;
}

static void WriteCallback(s32 chan, s32 result)
{
    CARDControl* card = &__CARDBlock[chan];
    CARDCallback callback;

    if (result >= 0) {
        CARDDir* dir0 = (CARDDir*)((u8*)card->workArea + 0x2000);
        CARDDir* dir1 = (CARDDir*)((u8*)card->workArea + 0x4000);

        if (card->currentDir == dir0) {
            card->currentDir = dir1;
            memcpy(dir1, dir0, 0x2000);
        } else {
            card->currentDir = dir0;
            memcpy(dir0, dir1, 0x2000);
        }
    }

    if (card->apiCallback == NULL) {
        __CARDPutControlBlock(card, result);
    }

    callback = card->eraseCallback;
    if (callback != NULL) {
        card->eraseCallback = NULL;
        callback(chan, result);
    }
}

static void EraseCallback(s32 chan, s32 result)
{
    CARDControl* card = &__CARDBlock[chan];
    CARDCallback callback;
    CARDDir* dir;
    /* This SDK build reserves eight unused bytes in the callback's stack frame. */
    u64 stackPadding;
    u32 address;

    if (result >= 0) {
        dir = __CARDGetDirBlock(card);
        address = ((u32)dir - (u32)card->workArea) / 0x2000 * card->sectorSize;
        result = __CARDWrite(chan, address, 0x2000, dir, WriteCallback);
        if (result >= 0) {
            return;
        }
    }

    if (card->apiCallback == NULL) {
        __CARDPutControlBlock(card, result);
    }

    callback = card->eraseCallback;
    if (callback != NULL) {
        card->eraseCallback = NULL;
        callback(chan, result);
    }
}

s32 __CARDUpdateDir(s32 chan, CARDCallback callback)
{
    CARDControl* card;
    CARDDirCheck* check;
    /* This SDK build reserves eight unused bytes in the update stack frame. */
    u64 stackPadding;
    u32 address;
    CARDDir* dir;

    card = &__CARDBlock[chan];
    if (!card->attached) {
        return CARD_RESULT_NOCARD;
    }

    dir = __CARDGetDirBlock(card);
    check = CARDGetDirCheck(dir);
    ++check->checkCode;
    __CARDCheckSum(dir, 0x2000 - sizeof(u32), &check->checkSum,
                   &check->checkSumInv);
    DCStoreRange(dir, 0x2000);

    card->eraseCallback = callback;
    address = ((u32)dir - (u32)card->workArea) / 0x2000 * card->sectorSize;
    return __CARDEraseSector(chan, address, EraseCallback);
}
