#include "dolphin/card.h"

#include "__card.h"

s32 __CARDReadSegment(s32 chan, CARDCallback callback);
s32 __CARDWritePage(s32 chan, CARDCallback callback);

static void BlockReadCallback(s32 chan, s32 result)
{
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];

    if (result >= 0) {
        card->xferred += 0x200;
        card->addr += 0x200;
        card->buffer = (u8*)card->buffer + 0x200;

        if (--card->repeat > 0) {
            result = __CARDReadSegment(chan, BlockReadCallback);
            if (result >= 0) {
                return;
            }
        }
    }

    if (card->apiCallback == NULL) {
        __CARDPutControlBlock(card, result);
    }

    callback = card->xferCallback;
    if (callback != NULL) {
        card->xferCallback = NULL;
        callback(chan, result);
    }
}

s32 __CARDRead(s32 chan, u32 address, s32 length, void* destination,
               CARDCallback callback)
{
    CARDControl* card;

    card = &__CARDBlock[chan];
    if (!card->attached) {
        return CARD_RESULT_NOCARD;
    }

    card->xferCallback = callback;
    card->repeat = (u32)length / 0x200;
    card->addr = address;
    card->buffer = destination;
    return __CARDReadSegment(chan, BlockReadCallback);
}

static void BlockWriteCallback(s32 chan, s32 result)
{
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];

    if (result >= 0) {
        card->xferred += card->pageSize;
        card->addr += card->pageSize;
        card->buffer = (u8*)card->buffer + card->pageSize;

        if (--card->repeat > 0) {
            result = __CARDWritePage(chan, BlockWriteCallback);
            if (result >= 0) {
                return;
            }
        }
    }

    if (card->apiCallback == NULL) {
        __CARDPutControlBlock(card, result);
    }

    callback = card->xferCallback;
    if (callback != NULL) {
        card->xferCallback = NULL;
        callback(chan, result);
    }
}

s32 __CARDWrite(s32 chan, u32 address, s32 length, void* source,
                CARDCallback callback)
{
    CARDControl* card;

    card = &__CARDBlock[chan];
    if (!card->attached) {
        return CARD_RESULT_NOCARD;
    }

    card->xferCallback = callback;
    card->repeat = length / card->pageSize;
    card->addr = address;
    card->buffer = source;
    return __CARDWritePage(chan, BlockWriteCallback);
}
