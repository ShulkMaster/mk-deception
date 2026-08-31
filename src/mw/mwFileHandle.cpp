struct mwFileTypeInfo {
};

class mwFile;
class mwFileCommand;
class mwFileDevice;
class mwFileMountPoint;
class mwFileServer;

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

enum mwFileSeekOrigin {
    MWF_SEEK_START,
    MWF_SEEK_CURRENT,
    MWF_SEEK_END
};

class mwFileQueryable {
public:
    mwFileQueryable()
    {
    }

    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileMultithreadedMemTraits {
public:
    static void deallocate(void*);
};

class mwFileCommand : public mwFileQueryable {
public:
    virtual ~mwFileCommand();
};

class mwFileServer {
public:
    virtual ~mwFileServer();
    virtual unsigned char isInitialized() const;
    virtual void service();
    virtual int addCommand(mwFileCommand*, unsigned char);
};

class mwFileMountPoint : public mwFileQueryable {
public:
    virtual ~mwFileMountPoint();
    virtual int startOpenFileCommand(
        mwFileCommand*&, const char*, unsigned long,
        mwFileCallback, void*) = 0;
    virtual int convertToPlatformPath(
        char*, const char*, unsigned long, unsigned long&) = 0;
    virtual const char* getInternalPath() const = 0;
    virtual mwFileServer& getServer() = 0;

    volatile int& referenceCount()
    {
        return reference_count;
    }

private:
    unsigned char field_0x04[0x24];
    volatile int reference_count;
};

template <class T>
class mwFileClassTypeInfo {
public:
    static mwFileTypeInfo sTypeInfo;
};

template <class T>
mwFileTypeInfo mwFileClassTypeInfo<T>::sTypeInfo;

class mwFileDevice {
};

class mwFile : public mwFileQueryable {
public:
    mwFile(mwFileMountPoint&, unsigned long);
    virtual ~mwFile() = 0;

    virtual int startCloseFileCommand(
        mwFileCommand*&, mwFileCallback, void*) = 0;
    virtual int startReadFileCommand(
        mwFileCommand*&, unsigned long long, void*, unsigned long,
        unsigned char, mwFileCallback, void*) = 0;
    virtual int startWriteFileCommand(
        mwFileCommand*&, unsigned long long, const void*, unsigned long,
        unsigned char, mwFileCallback, void*) = 0;
    virtual int startFlushFileCommand(
        mwFileCommand*&, mwFileCallback, void*) = 0;

    void updateSize(unsigned long long);
    int seek(long long, mwFileSeekOrigin);
    int addCommand(mwFileCommand*&, unsigned char);
    mwFileDevice* getDevice() const;
    void setDebugName(const char*);
    const char* getDebugName() const;

    static void operator delete(void* object)
    {
        if (object != 0) {
            mwFileMultithreadedMemTraits::deallocate(object);
        }
    }

private:
    unsigned long field_0x04;
    unsigned long long size;
    unsigned long long position;
    int field_0x18;
    int error;
    mwFileMountPoint* mount_point;
    unsigned long flags;
};

typedef char mwFileSizeCheck[sizeof(mwFile) == 0x28 ? 1 : -1];

extern void _mwFileNoOp(...);
extern int _mwFilePlatformInterlockedIncrement(volatile int&);
extern int _mwFilePlatformInterlockedDecrement(volatile int&);

static const char stringBase0[] =
    "mwFile: ERROR - seek tried to go beyond beginning of %s in seek mode "
    "MWF_SEEK_START\n\0\0"
    "mwFile: ERROR - seek of %d bytes tried to go beyond beginning of %s in "
    "seek mode MWF_SEEK_CURRENT\n\0"
    "mwFile: ERROR - seek of %d bytes exceeded end of %s in seek mode "
    "MWF_SEEK_END\n\0"
    "mwFile: ERROR - seek of %d bytes tried to go beyond beginning of %s\n\0"
    "mwFile: WARNING - seek of %d bytes past end of %s, which has size of %d "
    "bytes\n";

typedef char stringBase0SizeCheck[sizeof(stringBase0) == 0x19C ? 1 : -1];

enum {
    SEEK_START_MESSAGE = 0,
    EMPTY_DEBUG_NAME = 0x55,
    SEEK_CURRENT_MESSAGE = 0x56,
    SEEK_END_EXCEEDED_MESSAGE = 0xB9,
    SEEK_END_BEFORE_START_MESSAGE = 0x108,
    SEEK_PAST_END_MESSAGE = 0x14D
};

void mwFile::updateSize(unsigned long long new_size)
{
    size = new_size;
}

int mwFile::seek(long long offset, mwFileSeekOrigin origin)
{
    int result = 0;

    switch (origin) {
    case MWF_SEEK_START:
        if (offset >= 0) {
            position = offset;
        } else {
            _mwFileNoOp(
                &stringBase0[SEEK_START_MESSAGE],
                &stringBase0[EMPTY_DEBUG_NAME]);
            result = -9;
            position = 0;
        }
        break;

    case MWF_SEEK_CURRENT:
        if (offset >= 0) {
            position += offset;
        } else {
            offset = -offset;
            if (position < (unsigned long long)offset) {
                _mwFileNoOp(
                    &stringBase0[SEEK_CURRENT_MESSAGE], (int)offset,
                    &stringBase0[EMPTY_DEBUG_NAME]);
                result = -9;
                position = 0;
            } else {
                position -= offset;
            }
        }
        break;

    case MWF_SEEK_END:
        if (offset < 0) {
            _mwFileNoOp(
                &stringBase0[SEEK_END_EXCEEDED_MESSAGE], (int)offset,
                &stringBase0[EMPTY_DEBUG_NAME]);
            result = -9;
            position = size;
        } else if (size < (unsigned long long)offset) {
            _mwFileNoOp(
                &stringBase0[SEEK_END_BEFORE_START_MESSAGE], (int)offset,
                &stringBase0[EMPTY_DEBUG_NAME]);
            result = -9;
            position = 0;
        } else {
            position = size - offset;
        }
        break;
    }

    if (size < position) {
        _mwFileNoOp(
            &stringBase0[SEEK_PAST_END_MESSAGE], (int)offset,
            &stringBase0[EMPTY_DEBUG_NAME], (int)size);
        result = -9;
        position = size;
    }

    error = result;
    return result;
}

int mwFile::addCommand(mwFileCommand*& command, unsigned char priority)
{
    mwFileCommand* queued_command = command;
    int result;

    if (queued_command == 0) {
        return -10;
    }

    result = mount_point->getServer().addCommand(queued_command, priority);
    if (result != 0) {
        delete command;
        command = 0;
        return result;
    }
    return 0;
}

mwFileDevice* mwFile::getDevice() const
{
    void* queried_object;
    mwFileDevice* device;

    if (mount_point->isA(
            &mwFileClassTypeInfo<mwFileDevice>::sTypeInfo, queried_object)) {
        device = static_cast<mwFileDevice*>(queried_object);
    } else {
        device = 0;
    }

    if (device != 0) {
        return device;
    }
    return 0;
}

void mwFile::setDebugName(const char*)
{
}

const char* mwFile::getDebugName() const
{
    return &stringBase0[EMPTY_DEBUG_NAME];
}

mwFile::~mwFile()
{
    _mwFilePlatformInterlockedDecrement(mount_point->referenceCount());
}

mwFile::mwFile(mwFileMountPoint& owner, unsigned long open_flags)
    : size(0),
      position(0),
      field_0x18(0),
      error(0),
      mount_point(&owner),
      flags(open_flags)
{
    _mwFilePlatformInterlockedIncrement(owner.referenceCount());
}
