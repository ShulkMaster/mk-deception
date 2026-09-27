/* BUILD: -opt off clears inherited -O4,p from cflags_base before per-TU level */

#include "rw/rwplcore.h"

int _rwpathisabsolute(const char* path) {
    if (path[1] == ':') {
        if ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) {
            return 1;
        }
    }
    if (path[0] == '\\') {
        return 1;
    }
    return 0;
}
