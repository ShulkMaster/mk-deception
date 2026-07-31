#include "runtime/mk_hwfile.h"

void debug_file_close(MkHwFileRequest* file) {
    mk_hwfile_close(file);
}

int debug_file_write(MkHwFileRequest* file, void* buffer, int length) {
    return mk_hwfile_write_blocking(file, buffer, length);
}

MkHwFileRequest* debug_file_open(const char* path, const char* mode) {
    return mk_hwfile_open_blocking(path, mode);
}
