#include "dolphin/card.h"
#include "runtime/cstring.h"

#include "__card.h"

s32 __CARDGetStatusEx(s32 chan, s32 fileNo, CARDDir* status)
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
        memcpy(status, entry, sizeof(*status));
    }
    return __CARDPutControlBlock(card, result);
}

s32 __CARDSetStatusExAsync(s32 chan, s32 fileNo, CARDDir* status,
                           CARDCallback callback)
{
    CARDControl* card;
    CARDDir* dir;
    CARDDir* entry;
    s32 result;
    u8* name;
    s32 i;

    if (fileNo < 0 || fileNo >= CARD_MAX_FILE || status->fileName[0] == 0xFF ||
        status->fileName[0] == 0) {
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

    for (name = status->fileName; name < (u8*)&status->time; name++) {
        if (*name != 0) {
            continue;
        }
        while (++name < (u8*)&status->time) {
            *name = 0;
        }
        break;
    }

    if (status->permission & 0x20) {
        memset(status->gameName, 0, sizeof(status->gameName));
        memset(status->company, 0, sizeof(status->company));
    }
    if (status->permission & 0x40) {
        memset(status->gameName, 0, sizeof(status->gameName));
    }

    if (memcmp(entry->fileName, status->fileName, sizeof(entry->fileName)) != 0 ||
        memcmp(entry->gameName, status->gameName, sizeof(entry->gameName)) != 0 ||
        memcmp(entry->company, status->company, sizeof(entry->company)) != 0) {
        for (i = 0; i < CARD_MAX_FILE; i++) {
            CARDDir* candidate = &dir[i];

            if (i != fileNo && candidate->gameName[0] != 0xFF &&
                memcmp(candidate->gameName, status->gameName,
                       sizeof(candidate->gameName)) == 0 &&
                memcmp(candidate->company, status->company,
                       sizeof(candidate->company)) == 0 &&
                memcmp(candidate->fileName, status->fileName,
                       sizeof(candidate->fileName)) == 0) {
                return __CARDPutControlBlock(card, CARD_RESULT_EXIST);
            }
        }

        memcpy(entry->fileName, status->fileName, sizeof(entry->fileName));
        memcpy(entry->gameName, status->gameName, sizeof(entry->gameName));
        memcpy(entry->company, status->company, sizeof(entry->company));
    }

    entry->time = status->time;
    entry->bannerFormat = status->bannerFormat;
    entry->iconAddr = status->iconAddr;
    entry->iconFormat = status->iconFormat;
    entry->iconSpeed = status->iconSpeed;
    entry->commentAddr = status->commentAddr;
    entry->permission = status->permission;
    entry->copyTimes = status->copyTimes;

    result = __CARDUpdateDir(chan, callback);
    if (result < 0) {
        __CARDPutControlBlock(card, result);
    }
    return result;
}
