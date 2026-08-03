#include "mw/mwFile.h"

int mwFileClose(_mwFile* file) {
    mwFileCommand* command;
    mwFileAsyncResult result;

    command = mwFileCloseAsync(file, 0, 0);
    if (command == 0) {
        return -2;
    }

    result = mwFileWaitForCompletion(command);
    mwFileFreeCommand(command);
    return (int)result.value.bytes;
}

_mwFile* mwFileOpen(const char* path, int flags) {
    mwFileCommand* command;
    mwFileAsyncResult result;

    command = mwFileOpenAsync(path, flags, 0, 0);
    if (command == 0) {
        return 0;
    }

    result = mwFileWaitForCompletion(command);
    mwFileFreeCommand(command);
    return result.value.file;
}
