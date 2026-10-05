#include "dolphin/cache.h"
#include "dolphin/card.h"
#include "runtime/cstring.h"

#include "__card.h"

static void WriteCallback(s32 chan, s32 result);
static void EraseCallback(s32 chan, s32 result);

void* __CARDGetFatBlock(CARDControl* card)
{
    return card->currentFat;
}

static void WriteCallback(s32 chan, s32 result)
{
    CARDControl* card = &__CARDBlock[chan];
    CARDCallback callback;
    u16* fat0;
    u16* fat1;

    if (result >= 0) {
        fat0 = (u16*)((u8*)card->workArea + 3 * CARD_SYSTEM_BLOCK_SIZE);
        fat1 = (u16*)((u8*)card->workArea + 4 * CARD_SYSTEM_BLOCK_SIZE);

        if (card->currentFat == fat0) {
            card->currentFat = fat1;
            memcpy(fat1, fat0, CARD_SYSTEM_BLOCK_SIZE);
        } else {
            card->currentFat = fat0;
            memcpy(fat0, fat1, CARD_SYSTEM_BLOCK_SIZE);
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
    u16* fat;
    u32 offset;
    u32 address;

    if (result >= 0) {
        fat = __CARDGetFatBlock(card);
        offset = (u8*)fat - (u8*)card->workArea;
        address = card->sectorSize * (offset / CARD_SYSTEM_BLOCK_SIZE);
        result = __CARDWrite(chan, address, CARD_SYSTEM_BLOCK_SIZE, fat,
                             WriteCallback);
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

s32 __CARDAllocBlock(s32 chan, u32 cBlock, CARDCallback callback)
{
    CARDControl* card;
    u16* fat;
    u16 iBlock;
    u16 startBlock;
    u16 previousBlock;
    u16 count;

    card = &__CARDBlock[chan];
    if (!card->attached) {
        return CARD_RESULT_NOCARD;
    }

    fat = __CARDGetFatBlock(card);
    if (fat[CARD_FAT_FREEBLOCKS] < cBlock) {
        return CARD_RESULT_INSSPACE;
    }

    fat[CARD_FAT_FREEBLOCKS] -= cBlock;
    startBlock = 0xFFFF;
    iBlock = fat[CARD_FAT_LASTSLOT];
    count = 0;
    while (cBlock > 0) {
        if (card->cBlock - CARD_NUM_SYSTEM_BLOCK < ++count) {
            return CARD_RESULT_BROKEN;
        }

        ++iBlock;
        if (!CARDIsValidBlockNo(card, iBlock)) {
            iBlock = CARD_NUM_SYSTEM_BLOCK;
        }

        if (fat[iBlock] == 0) {
            if (startBlock == 0xFFFF) {
                startBlock = iBlock;
            } else {
                fat[previousBlock] = iBlock;
            }
            previousBlock = iBlock;
            fat[iBlock] = 0xFFFF;
            --cBlock;
        }
    }

    fat[CARD_FAT_LASTSLOT] = iBlock;
    card->startBlock = startBlock;
    return __CARDUpdateFatBlock(chan, fat, callback);
}

s32 __CARDFreeBlock(s32 chan, u16 block, CARDCallback callback)
{
    u16* fat;
    u16 nextBlock;

    if (!__CARDBlock[chan].attached) {
        return CARD_RESULT_NOCARD;
    }

    fat = __CARDGetFatBlock(&__CARDBlock[chan]);
    while (block != 0xFFFF) {
        if (!CARDIsValidBlockNo(&__CARDBlock[chan], block)) {
            return CARD_RESULT_BROKEN;
        }

        nextBlock = fat[block];
        fat[block] = 0;
        block = nextBlock;
        ++fat[CARD_FAT_FREEBLOCKS];
    }

    return __CARDUpdateFatBlock(chan, fat, callback);
}

s32 __CARDUpdateFatBlock(s32 chan, u16* fat, CARDCallback callback)
{
    CARDControl* card;
    u32 address;

    card = &__CARDBlock[chan];
    ++fat[CARD_FAT_CHECKCODE];
    __CARDCheckSum(fat + CARD_FAT_CHECKCODE, CARD_SYSTEM_BLOCK_SIZE - sizeof(u32),
                   fat + CARD_FAT_CHECKSUM, fat + CARD_FAT_CHECKSUMINV);
    DCStoreRange(fat, CARD_SYSTEM_BLOCK_SIZE);

    card->eraseCallback = callback;
    address = ((u8*)fat - (u8*)card->workArea) /
              CARD_SYSTEM_BLOCK_SIZE * card->sectorSize;
    return __CARDEraseSector(chan, address, EraseCallback);
}
