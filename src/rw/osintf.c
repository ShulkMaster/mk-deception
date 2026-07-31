#include "rw/rwplcore.h"

RwBool _rwpathisabsolute(const RwChar* path) {
    if (path[1] == ':') {
        if ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) {
            return TRUE;
        }
    }
    if (path[0] == '\\') {
        return TRUE;
    }
    return FALSE;
}
