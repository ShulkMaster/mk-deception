#include "dolphin/types.h"
#include "mw/mwFile.h"

class mwFileMountPoint;
class mwFileServer;
struct mwFileTypeInfo {
};

class mwFileMultithreadedMemTraits {
public:
    static void* allocate(size_t, mwTargetMemAlign, const char*);
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

    static void* operator new(size_t size, mwTargetMemAlign alignment,
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
        mwFileCommand*&, const char*, u32,
        mwFileCallback, void*) = 0;
};

class mwFile : public mwFileQueryable {
public:
    virtual ~mwFile();
    virtual int startCloseFileCommand(
        mwFileCommand*&, mwFileCallback, void*) = 0;
    virtual int startReadFileCommand(
        mwFileCommand*&, unsigned long long, void*, u32,
        int, mwFileCallback, void*) = 0;
    virtual int startWriteFileCommand(
        mwFileCommand*&, unsigned long long, const void*, u32,
        int, mwFileCallback, void*) = 0;
    virtual int startFlushFileCommand(
        mwFileCommand*&, mwFileCallback, void*) = 0;

    const unsigned long long& getSize() const
    {
        return size;
    }

    const char* getDebugName() const;

private:
    u32 field_0x04;
    unsigned long long size;
    unsigned long long position;
    int field_0x18;
    int error;
    mwFileMountPoint* mount_point;
    u32 flags;
};

class mwFileMountTable {
public:
    static mwFileMountTable& get();
    int getMountPointFromPath(
        mwFileMountPoint*&, const char*&, const char*) const;
};

class mwFileCommandProgress {
};

template <class T>
class mwFileClassTypeInfo {
public:
    static mwFileTypeInfo sTypeInfo;
};

template <class T>
mwFileTypeInfo mwFileClassTypeInfo<T>::sTypeInfo;

template class mwFileClassTypeInfo<mwFileCommandProgress>;

template <u32 ResultValue, int ResultError>
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

typedef char mwFileCommandSizeCheck[sizeof(mwFileCommand) == 0x20 ? 1 : -1];
typedef char mwFileFailCommandSizeCheck[
    sizeof(mwFileFailCommand<0, -9>) == 0x24 ? 1 : -1];

extern mwFileServer* mwFileGetGenericCommandServer();
extern void _mwFileNoOp(...);

static const u32 MWF_OPEN_READ = 1;

template <u32 ResultValue, int ResultError>
inline mwFileFailCommand<ResultValue, ResultError>::~mwFileFailCommand()
{
}

template <u32 ResultValue, int ResultError>
inline unsigned char mwFileFailCommand<ResultValue, ResultError>::isCompleted(
    mwFileAsyncResult& result) const
{
    result.value.bytes = ResultValue;
    result.error = ResultError;
    return completed;
}

template <u32 ResultValue, int ResultError>
inline unsigned char mwFileFailCommand<ResultValue, ResultError>::isAborted()
    const
{
    return 0;
}

template <u32 ResultValue, int ResultError>
inline mwFileAsyncResult
mwFileFailCommand<ResultValue, ResultError>::waitForCompletion() const
{
    mwFileAsyncResult result;
    while (!isCompleted(result)) {
        mwFileTick();
    }
    return result;
}

template <u32 ResultValue, int ResultError>
inline int mwFileFailCommand<ResultValue, ResultError>::abort()
{
    return 0;
}

template <u32 ResultValue, int ResultError>
inline int mwFileFailCommand<ResultValue, ResultError>::service()
{
    return 2;
}

template <u32 ResultValue, int ResultError>
inline void mwFileFailCommand<ResultValue, ResultError>::serviceCallback()
{
    completed = true;
    handleCompletion(waitForCompletion());
}

template <u32 ResultValue, int ResultError>
inline int mwFileFailCommand<ResultValue, ResultError>::close()
{
    return 0;
}

template <u32 ResultValue, int ResultError>
inline void mwFileFailCommand<ResultValue, ResultError>::deleteSelf()
{
    if (!completed || callback_depth != 0) {
        pending_delete = 1;
    } else {
        close();
        delete this;
    }
}

static int fn_stub1(void)
{
    _mwFileNoOp("mwFile: ERROR - calling mwFileGetCommandProgress on a command that can "
                "not report progress\n");
    _mwFileNoOp("mwFile: ERROR - failure to read command progress\n");
    return -1;
}

void mwFileFreeCommand(mwFileCommand* command)
{
    if (command != 0) {
        command->deleteSelf();
    }
}

mwFileAsyncResult mwFileWaitForCompletion(
    mwFileCommand* command)
{
    mwFileAsyncResult result;
    result = command->waitForCompletion();
    return result;
}

unsigned char mwFileIsCommandCompleted(
    mwFileCommand* command, mwFileAsyncResult* result)
{
    if (command->isCompleted(*result)) {
        mwFileTick();
        return 1;
    }
    return 0;
}

int mwFileAbortCommand(mwFileCommand* command)
{
    int result = command->abort();
    if (result != 0) {
        _mwFileNoOp("mwFile: ERROR - failure aborting command\n");
        return result;
    }
    return 0;
}

mwFileCommand* mwFileWriteAsync(
    mwFile* file, long long offset, void* buffer, u32 length,
    int priority, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result;

    if (length == 0) {
        command = new (MW_TARGET_MEM_ALIGN_32, "zero byte read/write command")
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
        _mwFileNoOp("mwFile: ERROR - failure starting async write: %d\n", result);
        return 0;
    }
    return command;
}

mwFileCommand* mwFileReadAsync(
    mwFile* file, unsigned long long offset, void* buffer, u32 length,
    int priority, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result;
    unsigned long long file_size;

    if (file == 0) {
        _mwFileNoOp("mwFile: ERROR - passed a NULL file handle in mwFileReadAsync\n");
        return 0;
    }
    if (buffer == 0) {
        _mwFileNoOp("mwFile: ERROR - passed a NULL buffer in mwFileReadAsync\n");
        return 0;
    }

    file_size = file->getSize();
    if (file_size < offset) {
        _mwFileNoOp(
            "mwFile: ERROR - file read out of range (%s, offset - %d, file size - %d)\n", file->getDebugName(),
            (int)offset, file_size);
        command = new (MW_TARGET_MEM_ALIGN_32,
                       "out of range read command")
            mwFileFailCommand<0, -9>(
                completion_callback, completion_data, file);
        if (command == 0) {
            result = -10;
        } else {
            result = mwFileGetGenericCommandServer()->addCommand(
                command, priority & 3);
        }
    } else {
        if (file_size < offset + length) {
            u32 truncated_length =
                file_size - offset;
            _mwFileNoOp(
                "mwFile: INFO - truncating read beyond end of file (%s, offset - %d): was %d bytes, now %d bytes\n", file->getDebugName(),
                (int)offset, length, truncated_length);
            length = truncated_length;
        }

        if (length == 0) {
            command = new (
                MW_TARGET_MEM_ALIGN_32, "zero byte read/write command")
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
        _mwFileNoOp("mwFile: ERROR - failure starting async read: %d\n", result);
        return 0;
    }
    return command;
}

extern "C" mwFileCommand* mwFileFlushAsync(
    mwFile* file, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result = file->startFlushFileCommand(
        command, completion_callback, completion_data);

    if (result != 0) {
        _mwFileNoOp("mwFile: ERROR - failure starting async flush: %d\n", result);
        return 0;
    }
    return command;
}

mwFileCommand* mwFileCloseAsync(
    mwFile* file, mwFileCallback completion_callback, void* completion_data)
{
    mwFileCommand* command;
    int result = file->startCloseFileCommand(
        command, completion_callback, completion_data);

    if (result != 0) {
        _mwFileNoOp("mwFile: ERROR - failure starting async close: %d\n", result);
        return 0;
    }
    return command;
}

mwFileCommand* mwFileOpenAsync(
    const char* path, int flags, mwFileCallback completion_callback,
    void* completion_data)
{
    mwFileCommand* command;
    mwFileMountPoint* mount_point;
    const char* relative_path;

    u32 read_mode = flags & 1;
    if (read_mode != 0 && (flags & 2) != 0) {
        _mwFileNoOp("mwFile - WARNING - opening files for reading AND writing not supported, buffers may not be consistent - %s\n", path);
    }
    if (read_mode == 0 && (flags & 2) == 0) {
        _mwFileNoOp("mwFile - WARNING - neither MWF_OPEN_READ nor MWF_OPEN_WRITE provided to mwFileOpenAsync - defaulting to MWF_OPEN_READ on file %s\n", path);
        flags |= MWF_OPEN_READ;
    }

    _mwFileNoOp(
        "mwFile: INFO - mwFileOpenAsync(%s, 0x%x, %p, %p)\n", path, flags,
        completion_callback, completion_data);

    int result = mwFileMountTable::get().getMountPointFromPath(
        mount_point, relative_path, path);
    if (result != 0) {
        _mwFileNoOp("mwFile: ERROR - couldn't find mount point for async open of %s\n", path);
        return 0;
    }

    result = mount_point->startOpenFileCommand(
        command, relative_path, flags,
        completion_callback, completion_data);
    if (result != 0) {
        _mwFileNoOp("mwFile: ERROR - failure starting async open: %d\n", result);
        return 0;
    }
    return command;
}
