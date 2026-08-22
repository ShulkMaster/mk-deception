#include "dolphin/card.h"

#include "__card.h"

u16 __CARDVendorID = 0xFFFF;
u8 __CARDPermMask = 0x1C;

s32 CARDGetSerialNo(s32 chan, u64* serialNo)
{
    CARDControl* card;
    CARDID* id;
    u64 code;
    s32 result;
    int i;

    if (chan < 0 || chan >= 2) {
        return CARD_RESULT_FATAL_ERROR;
    }
    result = __CARDGetControlBlock(chan, &card);
    if (result < 0) {
        return result;
    }

    id = (CARDID*)card->workArea;
    for (code = 0, i = 0; i < sizeof(id->serial) / sizeof(u64); ++i) {
        code ^= *(u64*)&id->serial[sizeof(u64) * i];
    }
    *serialNo = code;
    return __CARDPutControlBlock(card, CARD_RESULT_READY);
}

#define CARDCheckAttr(attr, flag) ((u32)((attr) & (flag)) != 0)

s32 CARDSetAttributesAsync(s32 chan, s32 fileNo, u8 attributes,
                           CARDCallback callback)
{
    CARDDir entry;
    s32 result;

    if (attributes & ~__CARDPermMask) {
        return CARD_RESULT_NOPERM;
    }
    result = __CARDGetStatusEx(chan, fileNo, &entry);
    if (result < 0) {
        return result;
    }
    if ((CARDCheckAttr(entry.permission, 0x20) &&
         !CARDCheckAttr(attributes, 0x20)) ||
        (CARDCheckAttr(entry.permission, 0x40) &&
         !CARDCheckAttr(attributes, 0x40))) {
        return CARD_RESULT_NOPERM;
    }
    if ((CARDCheckAttr(attributes, 0x20) &&
         CARDCheckAttr(attributes, 0x40)) ||
        (CARDCheckAttr(attributes, 0x20) &&
         CARDCheckAttr(entry.permission, 0x40)) ||
        (CARDCheckAttr(attributes, 0x40) &&
         CARDCheckAttr(entry.permission, 0x20))) {
        return CARD_RESULT_NOPERM;
    }

    entry.permission = attributes;
    return __CARDSetStatusExAsync(chan, fileNo, &entry, callback);
}

s32 CARDSetAttributes(s32 chan, s32 fileNo, u8 attributes)
{
    s32 result = CARDSetAttributesAsync(chan, fileNo, attributes,
                                        __CARDSyncCallback);
    if (result < 0) {
        return result;
    }
    return __CARDSync(chan);
}
