#include "dolphin/dvd.h"
#include "runtime/cstring.h"

int DVDCompareDiskID(const DVDDiskID* first, const DVDDiskID* second)
{
    if (first->gameName[0] != 0 && second->gameName[0] != 0 &&
        strncmp(first->gameName, second->gameName, 4) != 0) {
        return 0;
    }

    if (first->company[0] == 0 || second->company[0] == 0 ||
        strncmp(first->company, second->company, 2) != 0) {
        return 0;
    }

    if (first->diskNumber != 0xFF && second->diskNumber != 0xFF &&
        first->diskNumber != second->diskNumber) {
        return 0;
    }

    if (first->gameVersion != 0xFF && second->gameVersion != 0xFF &&
        first->gameVersion != second->gameVersion) {
        return 0;
    }

    return 1;
}
