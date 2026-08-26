#include "runtime/cfile.h"

enum MSLFileKind {
    MSL_FILE_CLOSED,
    MSL_FILE_DISK,
    MSL_FILE_CONSOLE,
    MSL_FILE_UNAVAILABLE,
};

enum MSLFileOrientation {
    MSL_FILE_UNORIENTED,
    MSL_FILE_CHAR_ORIENTED,
    MSL_FILE_WIDE_ORIENTED,
};

typedef struct MSLFileMode {
    unsigned int open_mode : 2;
    unsigned int io_mode : 3;
    unsigned int buffer_mode : 2;
    unsigned int file_kind : 3;
    unsigned int file_orientation : 2;
    unsigned int binary_io : 1;
} MSLFileMode;

struct FILE {
    unsigned long handle;
    MSLFileMode mode;
};

int fwide(FILE* file, int mode)
{
    if (file == 0 || file->mode.file_kind == MSL_FILE_CLOSED) {
        return 0;
    }

    switch (file->mode.file_orientation) {
    case MSL_FILE_UNORIENTED:
        if (mode > 0) {
            file->mode.file_orientation = MSL_FILE_WIDE_ORIENTED;
        } else if (mode < 0) {
            file->mode.file_orientation = MSL_FILE_CHAR_ORIENTED;
        }
        return mode;
    case MSL_FILE_WIDE_ORIENTED:
        return 1;
    case MSL_FILE_CHAR_ORIENTED:
        return -1;
    }
}
