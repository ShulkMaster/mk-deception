#include "rw/rwplcore.h"
#include "dolphin/gx.h"

RwUInt16 _RwDlTokenCurrent = 1;
RwUInt16 _RwDlTokenLastSeen;

RwBool _rwDlTokenQueryDone(RwUInt16 token) {
    _RwDlTokenLastSeen = GXReadDrawSync();
    if (_RwDlTokenLastSeen >= 0xE000) {
        return FALSE;
    }

    if (_RwDlTokenCurrent >= _RwDlTokenLastSeen) {
        RwBool done = TRUE;
        if (token > _RwDlTokenLastSeen && token <= _RwDlTokenCurrent) {
            done = FALSE;
        }
        return done;
    } else {
        RwBool done = FALSE;
        if (token > _RwDlTokenCurrent && token <= _RwDlTokenLastSeen) {
            done = TRUE;
        }
        return done;
    }
}
