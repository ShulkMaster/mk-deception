class mwFile;
class mwFileCommand;
class mwFileServer;

enum mwTargetMemAlign {
    MW_TARGET_MEM_ALIGN_DEFAULT = 0
};

enum mwFileSeekOrigin {
    MWF_SEEK_START,
    MWF_SEEK_CURRENT,
    MWF_SEEK_END
};

union mwFileAsyncValue {
    void* pointer;
    mwFile* file;
    unsigned long bytes;
};

struct mwFileAsyncResult {
    mwFileAsyncValue value;
    int error;
};

typedef void (*mwFileCallback)(
    mwFileCommand*, mwFileAsyncResult, void*);
typedef void (*mwFileTickCallback)(int);

struct _mwFileInitParam {
    unsigned long file_handle_tracking_size;
    unsigned short max_open_files;
    unsigned short field_0x06;
    unsigned short max_commands;
    unsigned short field_0x0A;
    unsigned long flags;
    unsigned short command_retries;
    unsigned short field_0x12;
    void* allocator_context;
    unsigned long field_0x18;
};

typedef _mwFileInitParam mwFileInitParam;

struct mwFileTypeInfo {
};

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFile : public mwFileQueryable {
public:
    int seek(long long, mwFileSeekOrigin);

    unsigned long long getSize() const
    {
        return size;
    }

    unsigned long long getPosition() const
    {
        return position;
    }

    void setError(int value)
    {
        error = value;
    }

private:
    unsigned long field_0x04;
    unsigned long long size;
    unsigned long long position;
    int field_0x18;
    int error;
    void* mount_point;
    unsigned long flags;
};

class mwFileCommand {
public:
    static unsigned short sRetries;
};

class mwFileDevice {
public:
    static void initializeCallbacks(unsigned long);
};

class mwFileServer {
};

class mwFileServerManagerBase {
public:
    void addServer(mwFileServer*);
    void removeServer(mwFileServer*);
};

class mwFileServerManagerMultiThreaded : public mwFileServerManagerBase {
public:
    mwFileServerManagerMultiThreaded();
    void initialize(unsigned long);
    void service();
    void serviceThread();
};

class mwFileServerNotQueued : public mwFileServer {
public:
    mwFileServerNotQueued();
    ~mwFileServerNotQueued();
    int initialize(unsigned short);

private:
    unsigned long field_0x00;
    unsigned char field_0x04[0x64];
};

class mwFileMutex {
public:
    mwFileMutex();
    void lock();
    void unlock();

private:
    unsigned long field_0x00;
    unsigned char field_0x04[0x14];
};

class mwFileMemTraits {
public:
    static void deallocate(void*);
    static void* allocate(
        unsigned long, mwTargetMemAlign, const char*);
};

union mwFileServerManagerStorage {
    unsigned long alignment;
    unsigned char bytes[0x8C];
};

union mwFileMutexStorage {
    unsigned long alignment;
    unsigned char bytes[0x18];
};

class mwFileMultithreadedMemTraits {
public:
    static void deallocate(void*);
    static void* allocate(
        unsigned long, mwTargetMemAlign, const char*);
    static mwFileMutex* getMutex();

private:
    static mwFileMutexStorage sMutexStorage;
    static mwFileMutex* spMemMutex;
};

inline void* operator new(unsigned long, void* storage)
{
    return storage;
}

extern void _mwFileNoOp(...);
extern void _mwFilePlatformTick();
extern bool _mwFilePlatformIsInServiceThread();
extern int _mwFilePlatformInit(mwFileInitParam*);
extern int _mwFileMountInit();

extern "C" char* strchr(const char*, int);

extern "C" mwFileCommand* mwFileCloseAsync(
    mwFile*, mwFileCallback, void*);
extern "C" mwFileCommand* mwFileOpenAsync(
    const char*, int, mwFileCallback, void*);
extern "C" mwFileAsyncResult mwFileWaitForCompletion(mwFileCommand*);
extern "C" void mwFileFreeCommand(mwFileCommand*);

static const char stringBase0[] =
    "mwFile: WARNING - profile data only available on metrics builds of "
    "mwFile\n\0"
    "mwFile: ERROR - invalid range for seek\n\0"
    "mwFile: ERROR - mwFile does not support reading and writing at the same "
    "time\n\0"
    "mwFile: ERROR -- couldn't initialize file handle tracking\n\0"
    "mwFile: ERROR - initializing mounting system: %d\n\0"
    "mwFile: ERROR - initializing platform-specific mwFile: %d\n";

enum {
    INVALID_SEEK_MESSAGE = 0x4B,
    INVALID_OPEN_MODE_MESSAGE = 0x73,
    TRACKING_INIT_MESSAGE = 0xC1,
    MOUNT_INIT_MESSAGE = 0xFC,
    PLATFORM_INIT_MESSAGE = 0x12E,
    MWF_OPEN_WRITE_MODE = 0x1A,
    MWF_OPEN_APPEND_MODE = 0x16
};

static const unsigned long MWF_OPEN_READ = 1;

static mwFileServerManagerStorage sServerMemory;
static mwFileServerNotQueued sGenericServer;

static mwFileServerManagerMultiThreaded* spServers;
static mwFileTickCallback spTickCallback;

mwFileMutexStorage mwFileMultithreadedMemTraits::sMutexStorage;
mwFileMutex* mwFileMultithreadedMemTraits::spMemMutex;

typedef char mwFileSizeCheck[sizeof(mwFile) == 0x28 ? 1 : -1];
typedef char mwFileInitParamSizeCheck[
    sizeof(mwFileInitParam) == 0x1C ? 1 : -1];
typedef char mwFileServerSizeCheck[
    sizeof(mwFileServerNotQueued) == 0x68 ? 1 : -1];
typedef char mwFileMutexSizeCheck[
    sizeof(mwFileMutex) == 0x18 ? 1 : -1];
typedef char stringBase0SizeCheck[sizeof(stringBase0) == 0x169 ? 1 : -1];

static inline void setDefaultInitParam(mwFileInitParam* parameters)
{
    if (parameters != 0) {
        parameters->file_handle_tracking_size = 0x2000;
        parameters->max_open_files = 0x10;
        parameters->field_0x06 = 0;
        parameters->max_commands =
            (unsigned short)(parameters->max_open_files * 2);
        parameters->flags = 3;
        parameters->allocator_context = 0;
        parameters->command_retries = 10;
        parameters->field_0x18 = 0;
    }
}

extern "C" unsigned long long mwFileGetSize(mwFile* file)
{
    return file->getSize();
}

extern "C" unsigned long long mwFileTell(mwFile* file)
{
    return file->getPosition();
}

extern "C" long long mwFileSeek(
    mwFile* file, long long offset, int origin)
{
    int result = file->seek(offset, (mwFileSeekOrigin)origin);
    file->setError(result);
    if (result != 0) {
        _mwFileNoOp(&stringBase0[INVALID_SEEK_MESSAGE]);
    }
    return file->getPosition();
}

extern "C" int mwFileClose(mwFile* file)
{
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

extern "C" mwFile* mwFileOpen(const char* path, int flags)
{
    mwFileCommand* command = mwFileOpenAsync(path, flags, 0, 0);
    if (command == 0) {
        return 0;
    }

    mwFileAsyncResult result = mwFileWaitForCompletion(command);
    mwFileFreeCommand(command);
    return result.value.file;
}

void _mwFileServiceThread()
{
    spServers->serviceThread();
}

void _mwFileTickEx(bool perform_callbacks)
{
    _mwFilePlatformTick();
    spServers->service();
    if (!_mwFilePlatformIsInServiceThread() && spTickCallback != 0) {
        spTickCallback(perform_callbacks);
    }
}

extern "C" void mwFileTick()
{
    _mwFilePlatformTick();
    spServers->service();
    if (!_mwFilePlatformIsInServiceThread() && spTickCallback != 0) {
        spTickCallback(false);
    }
}

extern "C" int mwFileOpenModeToFlags(const char* mode)
{
    const char* open_mode = mode;
    int flags;
    flags = 0;

    if (strchr(open_mode, 'r') != 0) {
        flags |= MWF_OPEN_READ;
    }
    if (strchr(open_mode, 'w') != 0) {
        flags |= MWF_OPEN_WRITE_MODE;
    }
    if (strchr(open_mode, 'a') != 0) {
        flags |= MWF_OPEN_APPEND_MODE;
    }
    if (strchr(open_mode, '+') != 0) {
        _mwFileNoOp(&stringBase0[INVALID_OPEN_MODE_MESSAGE]);
    }
    return flags;
}

extern "C" void mwFileGetDefaultInitParam(mwFileInitParam*);

extern "C" int mwFileInit(mwFileInitParam* parameters)
{
    mwFileInitParam default_parameters;
    int result;

    if (parameters == 0) {
        setDefaultInitParam(&default_parameters);
        parameters = &default_parameters;
    }

    mwFileCommand::sRetries = parameters->command_retries;
    mwFileDevice::initializeCallbacks(parameters->max_commands);

    if (spServers == 0) {
        spServers = new (sServerMemory.bytes)
            mwFileServerManagerMultiThreaded;
    }

    result = sGenericServer.initialize(parameters->max_commands);
    if (result != 0) {
        return result;
    }

    spServers->initialize(parameters->max_commands);
    spServers->addServer(&sGenericServer);

    result = _mwFileMountInit();
    if (result != 0) {
        _mwFileNoOp(&stringBase0[MOUNT_INIT_MESSAGE], result);
        return result;
    }

    result = _mwFilePlatformInit(parameters);
    if (result != 0) {
        _mwFileNoOp(&stringBase0[PLATFORM_INIT_MESSAGE], result);
        return result;
    }
    return 0;
}

extern "C" void mwFileGetDefaultInitParam(mwFileInitParam* parameters)
{
    setDefaultInitParam(parameters);
}

mwFileServerNotQueued* mwFileGetGenericCommandServer()
{
    return &sGenericServer;
}

int mwFileUnregisterFileServer(mwFileServer& server)
{
    spServers->removeServer(&server);
    return 0;
}

int mwFileRegisterFileServer(mwFileServer& server)
{
    spServers->addServer(&server);
    return 0;
}

void mwFileMultithreadedMemTraits::deallocate(void* object)
{
    mwFileMutex* mutex = getMutex();
    mutex->lock();
    mwFileMemTraits::deallocate(object);
    mutex->unlock();
}

void* mwFileMultithreadedMemTraits::allocate(
    unsigned long size, mwTargetMemAlign alignment, const char* name)
{
    mwFileMutex* mutex = getMutex();
    mutex->lock();
    void* allocation = mwFileMemTraits::allocate(size, alignment, name);
    mutex->unlock();
    return allocation;
}

mwFileMutex* mwFileMultithreadedMemTraits::getMutex()
{
    if (spMemMutex == 0) {
        spMemMutex = new (sMutexStorage.bytes) mwFileMutex;
    }
    return spMemMutex;
}
