struct mwFileAsyncResult {
    void* value;
    int error;
};

extern "C" void* mwFileCloseAsync(void* file, int flags, void* callback);
extern "C" void* mwFileOpenAsync(
    const char* path, int flags, void* callback, void* callback_data);
extern "C" mwFileAsyncResult mwFileWaitForCompletion(void* command);
extern "C" void mwFileFreeCommand(void* command);

extern "C" int mwFileClose(void* file) {
    void* command;
    mwFileAsyncResult result;

    command = mwFileCloseAsync(file, 0, 0);
    if (command == 0) {
        return -2;
    }

    result = mwFileWaitForCompletion(command);
    mwFileFreeCommand(command);
    return (int)result.value;
}

extern "C" void* mwFileOpen(const char* path, int flags) {
    void* command;
    mwFileAsyncResult result;

    command = mwFileOpenAsync(path, flags, 0, 0);
    if (command == 0) {
        return 0;
    }

    result = mwFileWaitForCompletion(command);
    mwFileFreeCommand(command);
    return result.value;
}
