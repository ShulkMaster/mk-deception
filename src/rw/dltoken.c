#include "rw/dltoken.h"
#include "dolphin/gx.h"

unsigned short _RwDlTokenCurrent = 1;
unsigned short _RwDlTokenLastSeen;

int _rwDlTokenQueryDone(unsigned short token) {
    _RwDlTokenLastSeen = GXReadDrawSync();
    if (_RwDlTokenLastSeen >= 0xE000) {
        return 0;
    }

    if (_RwDlTokenCurrent >= _RwDlTokenLastSeen) {
        int done = 1;
        if (token > _RwDlTokenLastSeen && token <= _RwDlTokenCurrent) {
            done = 0;
        }
        return done;
    } else {
        int done = 0;
        if (token > _RwDlTokenCurrent && token <= _RwDlTokenLastSeen) {
            done = 1;
        }
        return done;
    }
}
