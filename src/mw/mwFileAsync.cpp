class mwFile;
class mwFileCommand;
class mwFileMountPoint;
class mwFileServer;
struct mwFileTypeInfo {
};

enum mwTargetMemAlign {
    MW_TARGET_MEM_ALIGN_DEFAULT = 0,
    MW_TARGET_MEM_ALIGN_32 = 3
};

union mwFileAsyncValue {
    void* pointer;
    mwFile* file;
    unsigned long bytes;
};

struct _mwFileAsyncResult {
    mwFileAsyncValue value;
    int error;
};

typedef _mwFileAsyncResult mwFileAsyncResult;
typedef void (*mwFileCallback)(
    mwFileCommand*, mwFileAsyncResult, void*);

class mwFileMultithreadedMemTraits {
public:
    static void* allocate(unsigned long, mwTargetMemAlign, const char*);
    static void deallocate(void*);
};

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileCommand : public mwFileQueryable {
public:
    mwFileCommand(mwFileCallback, void*, mwFile*);
    virtual ~mwFileCommand();

    virtual unsigned char isCompleted(mwFileAsyncResult&) const = 0;
    virtual unsigned char isAborted() const = 0;
    virtual mwFileAsyncResult waitForCompletion() const = 0;
    virtual int abort() = 0;
    virtual int service() = 0;
    virtual void serviceCallback() = 0;
    virtual void deleteSelf() = 0;
    virtual int close() = 0;

    void handleCompletion(mwFileAsyncResult);

    static void* operator new(unsigned long size, mwTargetMemAlign alignment,
                              const char* name)
    {
        return mwFileMultithreadedMemTraits::allocate(
            size, alignment, name);
    }

    static void operator delete(void* object)
    {
        if (object != 0) {
            mwFileMultithreadedMemTraits::deallocate(object);
        }
    }

protected:
    mwFile* file;
    int error;
    unsigned char callback_depth;
    unsigned char pending_delete;
    unsigned char reserved_0E[2];
    volatile int reference_count;
    mwFileServer* server;
    mwFileCallback callback;
    void* callback_data;
};

class mwFileServer {
public:
    virtual ~mwFileServer();
    virtual unsigned char isInitialized() const;
    virtual void serviceCommands();
    virtual int addCommand(mwFileCommand*, unsigned char);
};

class mwFileMountPoint : public mwFileQueryable {
public:
    virtual ~mwFileMountPoint();
    virtual int startOpenFileCommand(
        mwFileCommand*&, const char*, unsigned long,
        mwFileCallback, void*) = 0;
};

class mwFile : public mwFileQueryable {
public:
    virtual ~mwFile();
    virtual int startCloseFileCommand(
        mwFileCommand*&, mwFileCallback, void*) = 0;
    virtual int startReadFileCommand(
        mwFileCommand*&, unsigned long long, void*, unsigned long,
        int, mwFileCallback, void*) = 0;
    virtual int startWriteFileCommand(
        mwFileCommand*&, unsigned long long, const void*, unsigned long,
        int, mwFileCallback, void*) = 0;
    virtual int startFlushFileCommand(
        mwFileCommand*&, mwFileCallback, void*) = 0;

    unsigned long long getSize() const
    {
        return size;
    }

    const char* getDebugName() const;

private:
    unsigned long field_0x04;
    unsigned long long size;
    unsigned long long position;
    int field_0x18;
    int error;
    mwFileMountPoint* mount_point;
    unsigned long flags;
};

class mwFileMountTable {
public:
    static mwFileMountTable& get();
    int getMountPointFromPath(
        mwFileMountPoint*&, const char*&, const char*) const;
};

class mwFileCommandProgress {
};

extern "C" void mwFileTick();

template <class T>
class mwFileClassTypeInfo {
public:
    static mwFileTypeInfo sTypeInfo;
};

template <class T>
mwFileTypeInfo mwFileClassTypeInfo<T>::sTypeInfo;

template class mwFileClassTypeInfo<mwFileCommandProgress>;

template <unsigned long ResultValue, int ResultError>
class mwFileFailCommand : public mwFileCommand {
public:
    mwFileFailCommand(mwFileCallback completion_callback,
                      void* completion_data, mwFile* owning_file)
        : mwFileCommand(completion_callback, completion_data, owning_file),
          completed(false)
    {
    }

    virtual ~mwFileFailCommand();
    virtual unsigned char isCompleted(mwFileAsyncResult&) const;
    virtual unsigned char isAborted() const;
    virtual mwFileAsyncResult waitForCompletion() const;
    virtual int abort();
    virtual int service();
    virtual void serviceCallback();
    virtual void deleteSelf();
    virtual int close();

private:
    bool completed;
};

template <>
class mwFileFailCommand<0, 0> : public mwFileCommand {
public:
    mwFileFailCommand(mwFileCallback completion_callback,
                      void* completion_data, mwFile* owning_file)
        : mwFileCommand(completion_callback, completion_data, owning_file),
          completed(false)
    {
    }

    virtual ~mwFileFailCommand();
    virtual unsigned char isCompleted(mwFileAsyncResult&) const;
    virtual unsigned char isAborted() const;
    virtual mwFileAsyncResult waitForCompletion() const;
    virtual int abort();
    virtual int service();
    virtual void serviceCallback();
    virtual void deleteSelf();
    virtual int close();

private:
    bool completed;
};

typedef char mwFileCommandSizeCheck[sizeof(mwFileCommand) == 0x20 ? 1 : -1];
typedef char mwFileFailCommandSizeCheck[
    sizeof(mwFileFailCommand<0, -9>) == 0x24 ? 1 : -1];

extern mwFileServer* mwFileGetGenericCommandServer();
extern void _mwFileNoOp(...);

static const unsigned long MWF_OPEN_READ = 1;

static const char stringBase0[] =
    "mwFile: ERROR - calling mwFileGetCommandProgress on a command that can "
    "not report progress\n\0"
    "mwFile: ERROR - failure to read command progress\n\0"
    "mwFile: ERROR - failure aborting command\n\0"
    "zero byte read/write command\0"
    "mwFile: ERROR - failure starting async write: %d\n\0"
    "mwFile: ERROR - passed a NULL file handle in mwFileReadAsync\n\0"
    "mwFile: ERROR - passed a NULL buffer in mwFileReadAsync\n\0"
    "mwFile: ERROR - file read out of range (%s, offset - %d, file size - "
    "%d)\n\0"
    "out of range read command\0"
    "mwFile: INFO - truncating read beyond end of file (%s, offset - %d): "
    "was %d bytes, now %d bytes\n\0"
    "mwFile: ERROR - failure starting async read: %d\n\0"
    "mwFile: ERROR - failure starting async flush: %d\n\0"
    "mwFile: ERROR - failure starting async close: %d\n\0"
    "mwFile - WARNING - opening files for reading AND writing not supported, "
    "buffers may not be consistent - %s\n\0"
    "mwFile - WARNING - neither MWF_OPEN_READ nor MWF_OPEN_WRITE provided to "
    "mwFileOpenAsync - defaulting to MWF_OPEN_READ on file %s\n\0"
    "mwFile: INFO - mwFileOpenAsync(%s, 0x%x, %p, %p)\n\0"
    "mwFile: ERROR - couldn't find mount point for async open of %s\n\0"
    "mwFile: ERROR - failure starting async open: %d\n";

typedef char stringBase0SizeCheck[sizeof(stringBase0) == 0x469 ? 1 : -1];

enum {
    ABORT_ERROR_MESSAGE = 0x8E,
    ZERO_LENGTH_NAME = 0xB8,
    WRITE_ERROR_MESSAGE = 0xD5,
    NULL_FILE_MESSAGE = 0x107,
    NULL_BUFFER_MESSAGE = 0x145,
    READ_RANGE_MESSAGE = 0x17E,
    RANGE_FAILURE_NAME = 0x1C8,
    READ_TRUNCATE_MESSAGE = 0x1E2,
    READ_ERROR_MESSAGE = 0x243,
    CLOSE_ERROR_MESSAGE = 0x2A6,
    OPEN_READ_WRITE_MESSAGE = 0x2D8,
    OPEN_NO_MODE_MESSAGE = 0x344,
    OPEN_INFO_MESSAGE = 0x3C6,
    OPEN_MOUNT_MESSAGE = 0x3F8,
    OPEN_ERROR_MESSAGE = 0x438
};

template <unsigned long ResultValue, int ResultError>
inline mwFileFailCommand<ResultValue, ResultError>::~mwFileFailCommand()
{
}

template <unsigned long ResultValue, int ResultError>
inline unsigned char mwFileFailCommand<ResultValue, ResultError>::isCompleted(
    mwFileAsyncResult& result) const
{
    result.value.bytes = ResultValue;
    result.error = ResultError;
    return completed;
}

template <unsigned long ResultValue, int ResultError>
inline unsigned char mwFileFailCommand<ResultValue, ResultError>::isAborted()
    const
{
    return 0;
}

template <unsigned long ResultValue, int ResultError>
inline mwFileAsyncResult
mwFileFailCommand<ResultValue, ResultError>::waitForCompletion() const
{
    mwFileAsyncResult result;
    while (!isCompleted(result)) {
        mwFileTick();
    }
    return result;
}

template <unsigned long ResultValue, int ResultError>
inline int mwFileFailCommand<ResultValue, ResultError>::abort()
{
    return 0;
}

template <unsigned long ResultValue, int ResultError>
inline int mwFileFailCommand<ResultValue, ResultError>::service()
{
    return 2;
}

template <unsigned long ResultValue, int ResultError>
inline void mwFileFailCommand<ResultValue, ResultError>::serviceCallback()
{
    completed = true;
    handleCompletion(waitForCompletion());
}

template <unsigned long ResultValue, int ResultError>
inline int mwFileFailCommand<ResultValue, ResultError>::close()
{
    return 0;
}

template <unsigned long ResultValue, int ResultError>
inline void mwFileFailCommand<ResultValue, ResultError>::deleteSelf()
{
    if (!completed || callback_depth != 0) {
        pending_delete = 1;
    } else {
        close();
        delete this;
    }
}

extern "C" void mwFileFreeCommand(mwFileCommand* command)
{
    if (command != 0) {
        command->deleteSelf();
    }
}

extern "C" mwFileAsyncResult mwFileWaitForCompletion(
    mwFileCommand* command)
{
    mwFileAsyncResult result;
    result = command->waitForCompletion();
    return result;
}

extern "C" unsigned char mwFileIsCommandCompleted(
    mwFileCommand* command, mwFileAsyncResult* result)
{
    if (command->isCompleted(*result)) {
        mwFileTick();
        return 1;
    }
    return 0;
}

extern "C" int mwFileAbortCommand(mwFileCommand* command)
{
    int result = command->abort();
    if (result != 0) {
        _mwFileNoOp(&stringBase0[ABORT_ERROR_MESSAGE]);
        return result;
    }
    return 0;
}

extern "C" mwFileCommand* mwFileWriteAsync(
    mwFile* file, long long offset, void* buffer, unsigned long length,
    int priority, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result;

    if (length == 0) {
        command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[ZERO_LENGTH_NAME])
            mwFileFailCommand<0, 0>(
                completion_callback, completion_data, file);
        if (command == 0) {
            result = -10;
        } else {
            result = mwFileGetGenericCommandServer()->addCommand(
                command, priority & 3);
        }
    } else {
        result = file->startWriteFileCommand(
            command, offset, buffer, length, priority,
            completion_callback, completion_data);
    }

    if (result != 0) {
        _mwFileNoOp(&stringBase0[WRITE_ERROR_MESSAGE], result);
        return 0;
    }
    return command;
}

extern "C" mwFileCommand* mwFileReadAsync(
    mwFile* file, long long offset, void* buffer, unsigned long length,
    int priority, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result;

    if (file == 0) {
        _mwFileNoOp(&stringBase0[NULL_FILE_MESSAGE]);
        return 0;
    }
    if (buffer == 0) {
        _mwFileNoOp(&stringBase0[NULL_BUFFER_MESSAGE]);
        return 0;
    }

    unsigned long long file_size = file->getSize();
    if (file_size < (unsigned long long)offset) {
        _mwFileNoOp(
            &stringBase0[READ_RANGE_MESSAGE], file->getDebugName(),
            (int)offset, file_size);
        command = new (MW_TARGET_MEM_ALIGN_32,
                       &stringBase0[RANGE_FAILURE_NAME])
            mwFileFailCommand<0, -9>(
                completion_callback, completion_data, file);
        if (command == 0) {
            result = -10;
        } else {
            result = mwFileGetGenericCommandServer()->addCommand(
                command, priority & 3);
        }
    } else {
        if (file_size < (unsigned long long)offset + length) {
            unsigned long truncated_length =
                (unsigned long)(file_size - offset);
            _mwFileNoOp(
                &stringBase0[READ_TRUNCATE_MESSAGE], file->getDebugName(),
                (int)offset, length, truncated_length);
            length = truncated_length;
        }

        if (length == 0) {
            command = new (
                MW_TARGET_MEM_ALIGN_32, &stringBase0[ZERO_LENGTH_NAME])
                mwFileFailCommand<0, 0>(
                    completion_callback, completion_data, file);
            if (command == 0) {
                result = -10;
            } else {
                result = mwFileGetGenericCommandServer()->addCommand(
                    command, priority & 3);
            }
        } else {
            result = file->startReadFileCommand(
                command, offset, buffer, length, priority,
                completion_callback, completion_data);
        }
    }

    if (result != 0) {
        _mwFileNoOp(&stringBase0[READ_ERROR_MESSAGE], result);
        return 0;
    }
    return command;
}

extern "C" mwFileCommand* mwFileCloseAsync(
    mwFile* file, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result = file->startCloseFileCommand(
        command, completion_callback, completion_data);

    if (result != 0) {
        _mwFileNoOp(&stringBase0[CLOSE_ERROR_MESSAGE], result);
        return 0;
    }
    return command;
}

extern "C" mwFileCommand* mwFileOpenAsync(
    const char* path, int flags, mwFileCallback completion_callback,
    void* completion_data)
{
    mwFileCommand* command;
    mwFileMountPoint* mount_point;
    const char* relative_path;

    unsigned long read_mode = flags & 1;
    if (read_mode != 0 && (flags & 2) != 0) {
        _mwFileNoOp(&stringBase0[OPEN_READ_WRITE_MESSAGE], path);
    }
    if (read_mode == 0 && (flags & 2) == 0) {
        _mwFileNoOp(&stringBase0[OPEN_NO_MODE_MESSAGE], path);
        flags |= MWF_OPEN_READ;
    }

    _mwFileNoOp(
        &stringBase0[OPEN_INFO_MESSAGE], path, flags,
        completion_callback, completion_data);

    int result = mwFileMountTable::get().getMountPointFromPath(
        mount_point, relative_path, path);
    if (result != 0) {
        _mwFileNoOp(&stringBase0[OPEN_MOUNT_MESSAGE], path);
        return 0;
    }

    result = mount_point->startOpenFileCommand(
        command, relative_path, flags,
        completion_callback, completion_data);
    if (result != 0) {
        _mwFileNoOp(&stringBase0[OPEN_ERROR_MESSAGE], result);
        return 0;
    }
    return command;
}
