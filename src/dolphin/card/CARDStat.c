#include "dolphin/card.h"
#include "runtime/cstring.h"

#include "__card.h"

static void UpdateIconOffsets(CARDDir* entry, CARDStat* stat)
{
    u32 offset;
    BOOL iconTlut;
    int i;

    offset = entry->iconAddr;
    if (offset == 0xffffffff) {
        stat->bannerFormat = 0;
        stat->iconFormat = 0;
        stat->iconSpeed = 0;
        offset = 0;
    }

    iconTlut = FALSE;
    switch (entry->bannerFormat & 3) {
    case 1:
        stat->offsetBanner = offset;
        offset += 96 * 32;
        stat->offsetBannerTlut = offset;
        offset += 2 * 256;
        break;
    case 2:
        stat->offsetBanner = offset;
        offset += 2 * 96 * 32;
        stat->offsetBannerTlut = (u32)-1;
        break;
    default:
        stat->offsetBanner = (u32)-1;
        stat->offsetBannerTlut = (u32)-1;
        break;
    }

    for (i = 0; i < CARD_ICON_MAX; ++i) {
        switch ((entry->iconFormat >> (2 * i)) & 3) {
        case 1:
            stat->offsetIcon[i] = offset;
            offset += 32 * 32;
            iconTlut = TRUE;
            break;
        case 2:
            stat->offsetIcon[i] = offset;
            offset += 2 * 32 * 32;
            break;
        default:
            stat->offsetIcon[i] = (u32)-1;
            break;
        }
    }

    if (iconTlut) {
        stat->offsetIconTlut = offset;
        offset += 2 * 256;
    } else {
        stat->offsetIconTlut = (u32)-1;
    }
    stat->offsetData = offset;
}

s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat* stat)
{
    CARDControl* card;
    CARDDir* dir;
    CARDDir* entry;
    s32 result;

    if (fileNo < 0 || fileNo >= CARD_MAX_FILE) {
        return CARD_RESULT_FATAL_ERROR;
    }
    result = __CARDGetControlBlock(chan, &card);
    if (result < 0) {
        return result;
    }
    dir = __CARDGetDirBlock(card);
    entry = &dir[fileNo];
    result = __CARDIsReadable(card, entry);
    if (result >= 0) {
        memcpy(stat->gameName, entry->gameName, sizeof(stat->gameName));
        memcpy(stat->company, entry->company, sizeof(stat->company));
        stat->length = (u32)entry->length * card->sectorSize;
        memcpy(stat->fileName, entry->fileName, CARD_FILENAME_MAX);
        stat->time = entry->time;
        stat->bannerFormat = entry->bannerFormat;
        stat->iconAddr = entry->iconAddr;
        stat->iconFormat = entry->iconFormat;
        stat->iconSpeed = entry->iconSpeed;
        stat->commentAddr = entry->commentAddr;
        UpdateIconOffsets(entry, stat);
    }
    return __CARDPutControlBlock(card, result);
}

s32 CARDSetStatusAsync(s32 chan, s32 fileNo, CARDStat* stat,
                       CARDCallback callback)
{
    CARDControl* card;
    CARDDir* dir;
    CARDDir* entry;
    s32 result;

    if (fileNo < 0 || fileNo >= CARD_MAX_FILE ||
        (stat->iconAddr != (u32)-1 && stat->iconAddr >= CARD_SEG_SIZE) ||
        (stat->commentAddr != (u32)-1 &&
         CARD_SYSTEM_BLOCK_SIZE - 64 <
             stat->commentAddr % CARD_SYSTEM_BLOCK_SIZE)) {
        return CARD_RESULT_FATAL_ERROR;
    }
    result = __CARDGetControlBlock(chan, &card);
    if (result < 0) {
        return result;
    }
    dir = __CARDGetDirBlock(card);
    entry = &dir[fileNo];
    result = __CARDIsWritable(card, entry);
    if (result < 0) {
        return __CARDPutControlBlock(card, result);
    }

    entry->bannerFormat = stat->bannerFormat;
    entry->iconAddr = stat->iconAddr;
    entry->iconFormat = stat->iconFormat;
    entry->iconSpeed = stat->iconSpeed;
    entry->commentAddr = stat->commentAddr;
    UpdateIconOffsets(entry, stat);
    if (entry->iconAddr == (u32)-1) {
        entry->iconSpeed = (entry->iconSpeed & ~3) | 1;
    }
    entry->time = OSGetTime() / (__OSBusClock / 4);
    result = __CARDUpdateDir(chan, callback);
    if (result < 0) {
        __CARDPutControlBlock(card, result);
    }
    return result;
}

s32 CARDSetStatus(s32 chan, s32 fileNo, CARDStat* stat)
{
    s32 result = CARDSetStatusAsync(chan, fileNo, stat, __CARDSyncCallback);
    if (result < 0) {
        return result;
    }
    return __CARDSync(chan);
}
