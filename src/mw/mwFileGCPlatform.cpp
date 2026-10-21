#include "dolphin/os.h"
#include "dolphin/dvd.h"
#include "ctype.h"
#include "mw/mwFile.h"
#include "mslcpp/msl_error.h"
#include "mslcpp/iterator.h"
#include "mslcpp/memory.h"
#include "dolphin/cache.h"
#include "runtime/cstring.h"

class mwFileMutex : public OSMutex {
public:
    mwFileMutex();
    ~mwFileMutex();
    void lock();
    void unlock();
};

class mwFileCondition : public OSCond {
public:
    mwFileCondition();
    ~mwFileCondition();
    void wait(mwFileMutex& mutex);
    void signal();
};

class mwFileEvent {
public:
    mwFileEvent();
    void reset();
    void set();

private:
    OSMutex mutex;
    OSCond condition;
    unsigned char signaled;
};

class mwFileOSMutexLock {
public:
    explicit mwFileOSMutexLock(OSMutex& value) : mutex(&value) { OSLockMutex(mutex); }
    ~mwFileOSMutexLock() { OSUnlockMutex(mutex); }

private:
    OSMutex* mutex;
    mwFileOSMutexLock(const mwFileOSMutexLock&);
    mwFileOSMutexLock& operator=(const mwFileOSMutexLock&);
};

static const char stringBase0[] =
    "/\0"
    "mwFile: ERROR - reading on /host not supported on GameCube, only writing... while opening file %s\n\0"
    "GCN host file handle\0"
    "mwFile: ERROR - could not wakeup file command!\n\0"
    "host\0"
    "cdrom\0"
    "Wakeup thread stack\0"
    "mwFile: Couldn't allocate wakeup thread stack\n\0"
    "mwFile: couldn't create command wakeup thread\n\0"
    "Service thread stack\0"
    "mwFile: Couldn't allocate service thread stack\n\0"
    "mwFile: couldn't create command service thread\n\0"
    "gcn cdrom mount point\0"
    "gcn host mount point\0"
    "mwFile flush command\0"
    "mwFile write command\0"
    "mwFile unbuffered read command\0"
    "mwFile: PERFORMANCE - buffered read (file: %s, offset: %d, size: %d)\n\0"
    "mwFile buffered read command\0"
    "mwFile close command\0"
    "mwFile - ERROR - trying to open file on DVD for write - %s\0"
    "mwFile: WARNING - error emulation on GameCube not supported, use GameCube DVD emulator to emulate errors\n\0"
    "basic_string::length error.  Resizing out of range\n\0"
    "STL allocation\0"
    "basic_string::reserve argument out of range\n\0"
    "mwFile read command\0"
    "mwFile open command\0"
    "string constructor: n > max_size\n\0"
    "mwFilePlatformHandle";

typedef char stringBase0SizeCheck[sizeof(stringBase0) == 0x3FC ? 1 : -1];

enum {
    SLASH_STRING = 0x0,
    HOST_READ_MESSAGE = 0x2,
    HOST_HANDLE_NAME = 0x65,
    WAKEUP_ERROR_MESSAGE = 0x7A,
    HOST_MOUNT_NAME = 0xAA,
    CDROM_MOUNT_NAME = 0xAF,
    WAKEUP_STACK_NAME = 0xB5,
    WAKEUP_STACK_MESSAGE = 0xC9,
    WAKEUP_THREAD_MESSAGE = 0xF8,
    SERVICE_STACK_NAME = 0x127,
    SERVICE_STACK_MESSAGE = 0x13C,
    SERVICE_THREAD_MESSAGE = 0x16C,
    CD_MOUNT_POINT_NAME = 0x19C,
    HOST_MOUNT_POINT_NAME = 0x1B2,
    FLUSH_COMMAND_NAME = 0x1C7,
    WRITE_COMMAND_NAME = 0x1DC,
    UNBUFFERED_READ_NAME = 0x1F1,
    BUFFERED_READ_MESSAGE = 0x210,
    BUFFERED_READ_NAME = 0x256,
    CLOSE_COMMAND_NAME = 0x273,
    DVD_WRITE_MESSAGE = 0x288,
    STL_ALLOCATION_NAME = 0x361,
    READ_COMMAND_NAME = 0x39D,
    OPEN_COMMAND_NAME = 0x3B1,
    STRING_CONSTRUCTOR_MESSAGE = 0x3C5,
    PLATFORM_HANDLE_NAME = 0x3E7
};

enum mwFileError {
    MWFILE_ERROR_NONE = 0,
    MWFILE_ERROR_ABORTED = -16,
    MWFILE_ERROR_COVER_OPEN = -19,
    MWFILE_ERROR_NO_DISK = -20,
    MWFILE_ERROR_WRONG_DISK = -21,
    MWFILE_ERROR_DRIVE = -22,
    MWFILE_ERROR_FATAL = -23
};

enum mwFileErrorState {
    MWFILE_ERROR_STATE_RAISED = 0,
    MWFILE_ERROR_STATE_CLEARED = 1
};

struct mwFileTypeInfo {
};

class mwFileServer;
class mwFileMountPoint;
class mwFile;
class mwFileDevice;
class mwFileBuffer;
class mwFileGCHandle;

class mwFileMultithreadedMemTraits {
public:
    static void* allocate(unsigned long size, mwTargetMemAlign alignment, const char* name);
    static void deallocate(void* memory);
};

template <class T>
bool mwFileMemAlloc(T*& memory, unsigned long size, mwTargetMemAlign alignment, const char* name);

extern void _mwFileNoOp(...);
extern void _mwFileServiceThread();
extern int mwFileRegisterFileServer(mwFileServer&);
extern int mwFileUnregisterFileServer(mwFileServer&);

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*& object) const
    {
        object = 0;
        return 0;
    }
};

template <class T>
class mwFileClassTypeInfo {
public:
    static mwFileTypeInfo sTypeInfo;
};

template <class T>
mwFileTypeInfo mwFileClassTypeInfo<T>::sTypeInfo;

class mwFileCommandProgress {
public:
    virtual ~mwFileCommandProgress() {}
    virtual int getProgressBytes(unsigned long&) = 0;
};

class mwFileGCErrorState {
public:
    virtual void queryErrorState() = 0;
};

class mwFileBuffer {
public:
    mwFileBuffer();
    ~mwFileBuffer();

    unsigned long readInRange(unsigned char*& destination, unsigned long& length,
                              unsigned long long& position);
    void reset(unsigned long long position, unsigned long length);
    void shutdown();
    int initialize(unsigned long size, mwTargetMemAlign alignment);

    unsigned long long getPosition() const { return file_position; }
    unsigned char* getData() const { return buffer; }
    unsigned long getLength() const { return buffer_end - buffer; }
    unsigned long getCapacity() const { return capacity; }
    bool isDirty() const { return dirty; }

private:
    unsigned long long file_position;
    unsigned long capacity;
    unsigned char* buffer;
    unsigned char* buffer_end;
    bool dirty;
};

class mwFileMountPoint;

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

    void wakeup();
    void handleCompletion(mwFileAsyncResult);

    mwFile* getFile() const { return file; }

    static void* operator new(unsigned long size, mwTargetMemAlign alignment,
                              const char* name)
    {
        return mwFileMultithreadedMemTraits::allocate(size, alignment, name);
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

class mwFileCommandPluginBase : public mwFileCommand {
public:
    mwFileCommandPluginBase(mwFileCallback, void*, mwFile*);

    virtual unsigned char isCompleted(mwFileAsyncResult&) const;
    virtual unsigned char isAborted() const;
    virtual mwFileAsyncResult waitForCompletion() const;
    virtual int abort();
    virtual int service() = 0;
    virtual void serviceCallback();
    virtual void deleteSelf();
    virtual int close();
    virtual mwFileAsyncResult getResult() const = 0;
    virtual ~mwFileCommandPluginBase() {}

    int handleSignalState();

protected:
    unsigned char aborted;
    volatile int signal_state;
};

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

typedef mwFileFailCommand<0, -13> mwFileNotSupportedCommand;

template <class T>
class mwFileCommandPlugin : public mwFileCommandPluginBase {
public:
    mwFileCommandPlugin(mwFileCallback completion_callback, void* completion_data,
                        mwFile* owning_file)
        : mwFileCommandPluginBase(completion_callback, completion_data, owning_file)
    {
    }

    virtual ~mwFileCommandPlugin();
    virtual int service();

    int serviceState(int state)
    {
        if (state == 2) {
            state = static_cast<T*>(this)->handleSignalState();
        }
        return state;
    }

protected:
    bool hasReferences() const { return reference_count > 0 ? true : false; }
};

template <class T>
mwFileCommandPlugin<T>::~mwFileCommandPlugin()
{
}

template <class T, class H>
class mwFileXPCommand : public mwFileCommandPlugin<T> {
public:
    mwFileXPCommand(mwFileCallback completion_callback, void* completion_data,
                    mwFile* owning_file)
        : mwFileCommandPlugin<T>(completion_callback, completion_data, owning_file)
    {
    }

    virtual ~mwFileXPCommand();
};

template <class T, class H>
mwFileXPCommand<T, H>::~mwFileXPCommand()
{
}

class mwFileServer {
public:
    mwFileServer();
    virtual ~mwFileServer();
    virtual bool isInitialized() const = 0;
    virtual int service() = 0;
    virtual int addCommand(mwFileCommand*, unsigned char) = 0;
    virtual bool isActive() const = 0;
    virtual bool queryActive() const = 0;

protected:
    void* manager;
    mwFileMutex mutex;
};

class mwFileServerQueued : public mwFileServer {
public:
    mwFileServerQueued();
    virtual ~mwFileServerQueued();
    virtual bool isInitialized() const;
    virtual int service();
    virtual int addCommand(mwFileCommand*, unsigned char);
    virtual bool isActive() const;
    virtual bool queryActive() const;

    int initialize(unsigned short command_count);
    void shutdown();

protected:
    mwFileCommand* active[3];
    void* pending;
    void* ready;
    int active_count;
    bool servicing;
};

class mwFileMountPoint : public mwFileQueryable {
public:
    mwFileMountPoint(const char* mount_name);
    virtual ~mwFileMountPoint();
    virtual int startOpenFileCommand(mwFileCommand*&, const char*, unsigned long,
                                     mwFileCallback, void*) = 0;
    virtual int convertToPlatformPath(char*, const char*, unsigned long,
                                      unsigned long&) = 0;
    virtual const char* getInternalPath() const = 0;
    virtual mwFileServer& getServer() = 0;

    const char* getName() const { return name; }
    volatile int& referenceCount() { return mount_count; }

    static void* operator new(unsigned long size, mwTargetMemAlign alignment,
                              const char* name)
    {
        return mwFileMultithreadedMemTraits::allocate(size, alignment, name);
    }

    static void operator delete(void* object)
    {
        if (object != 0) {
            mwFileMultithreadedMemTraits::deallocate(object);
        }
    }

protected:
    char name[36];
    volatile int mount_count;
};

class mwFileMountTable {
public:
    static mwFileMountTable& get();
    int getMountPointFromName(mwFileMountPoint*& mount_point, const char* name);
    int addMountPoint(mwFileMountPoint* mount_point);
};

extern "C" int mwFileUnmount(const char* mount);

typedef void (*mwFileDeviceErrorCallback)(mwFileErrorState, mwFileError, const char*,
                                          mwFileCommand*, void*);

class mwFileDevice {
public:
    mwFileDevice() {}
    virtual ~mwFileDevice();
    virtual void setErrorCallback(mwFileDeviceErrorCallback, void*) = 0;
    virtual void reportError(mwFileError, mwFileCommand*) = 0;
    virtual void revokeError(mwFileCommand*) = 0;

    struct Callback {
        mwFileDeviceErrorCallback function;
        void* callback_data;
        mwFileErrorState state;
        mwFileError error;
        const char* path;
        mwFileCommand* command;
    };

    static void queueErrorCallback(const Callback& callback);

protected:
    mwFileDeviceErrorCallback error_callback;
    void* error_callback_data;
    int error_count;
};

class mwFile : public mwFileQueryable {
public:
    mwFile(mwFileMountPoint&, unsigned long);
    virtual ~mwFile();
    virtual int startCloseFileCommand(mwFileCommand*&, mwFileCallback, void*) = 0;
    virtual int startReadFileCommand(mwFileCommand*&, unsigned long long, void*,
                                     unsigned long, unsigned char, mwFileCallback,
                                     void*) = 0;
    virtual int startWriteFileCommand(mwFileCommand*&, unsigned long long, const void*,
                                      unsigned long, unsigned char, mwFileCallback,
                                      void*) = 0;
    virtual int startFlushFileCommand(mwFileCommand*&, mwFileCallback, void*) = 0;

    void updateSize(unsigned long long);
    int addCommand(mwFileCommand*&, unsigned char);
    mwFileDevice* getDevice() const;
    void setDebugName(const char*);
    const char* getDebugName() const;

    unsigned long long getSize() const { return size; }
    unsigned long getFlags() const { return flags; }
    void setSize(unsigned long long value) { size = value; }

    static void* operator new(unsigned long size, mwTargetMemAlign alignment,
                              const char* name)
    {
        return mwFileMultithreadedMemTraits::allocate(size, alignment, name);
    }

    static void operator delete(void* object)
    {
        if (object != 0) {
            mwFileMultithreadedMemTraits::deallocate(object);
        }
    }

protected:
    unsigned long field_0x04;
    unsigned long long size;
    unsigned long long position;
    int field_0x18;
    int error;
    mwFileMountPoint* mount_point;
    unsigned long flags;
};

typedef char mwFileSizeCheck[sizeof(mwFile) == 0x28 ? 1 : -1];

namespace std {
template <class T>
struct char_traits {
    static void assign(T& c1, const T& c2) { c1 = c2; }
};

template <class Base, class Member>
struct _EmptyMemberOpt : public Base {
    _EmptyMemberOpt(const Base& b);
    _EmptyMemberOpt(const _EmptyMemberOpt& other);

    Member m_;
};

template <class Base, class Member>
_EmptyMemberOpt<Base, Member>::_EmptyMemberOpt(const _EmptyMemberOpt& other)
    : Base(other), m_(other.m_)
{
}
}

template <class T, int Alignment>
class mwFileMemAllocator : public std::allocator<T> {
public:
    typedef unsigned long size_type;
    typedef T* pointer;

    mwFileMemAllocator();
    mwFileMemAllocator(const mwFileMemAllocator&);
    ~mwFileMemAllocator() {}

    pointer allocate(size_type n)
    {
        if (n == 0) {
            n = 1;
        }
        return (pointer)mwFileMultithreadedMemTraits::allocate(
            n * sizeof(T), (mwTargetMemAlign)Alignment, &stringBase0[STL_ALLOCATION_NAME]);
    }
};

namespace std {
template <class T, class Traits, class Allocator>
class basic_string {
public:
    typedef unsigned long size_type;
    typedef T* iterator;

    explicit basic_string(const Allocator& a = Allocator());
    basic_string(const basic_string& str);
    basic_string(const T* s, const Allocator& a = Allocator());
    ~basic_string();

    void reserve(size_type size);
    basic_string& append(const T* text, size_type count);
    basic_string& operator=(const T* text);
    const T* data() const;
    iterator begin();
    iterator end();
    Allocator get_allocator() const;

    const T* c_str() const
    {
        static const T nullstring = T();
        if (data_ == 0) {
            return &nullstring;
        }
        return data_;
    }

private:
    void init(const T* begin, const T* end, const Allocator& a, random_access_iterator_tag);

    size_type size_;
    _EmptyMemberOpt<Allocator, size_type> alloc_;
    T* data_;
};

/* TODO: [near miss] 99.14815%; five tag-carrier/argument rows remain;
 * original size() boundary is neutral; recover temporary transport ownership. */
template <class T, class Traits, class Allocator>
basic_string<T, Traits, Allocator>::basic_string(const basic_string& str)
    : alloc_(str.alloc_)
{
    init(str.data_, str.data_ + str.size_, get_allocator(), random_access_iterator_tag());
}

template <class T, class Traits, class Allocator>
basic_string<T, Traits, Allocator>::basic_string(const T* s, const Allocator& a)
    : alloc_(a)
{
    unsigned long length = strlen(s);
    if (length > (unsigned long)-2) {
        __msl_error(&stringBase0[STRING_CONSTRUCTOR_MESSAGE]);
    }
    init(s, s + length, a, random_access_iterator_tag());
}

template <class T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::init(const T* begin, const T* end,
                                              const Allocator& a,
                                              random_access_iterator_tag)
{
    data_ = 0;
    size_ = 0;
    alloc_.m_ = 0;
    if (begin != end) {
        size_type n = end - begin;
        alloc_.m_ = n + 3 - n % 4;
        data_ = alloc_.allocate(alloc_.m_ + 1);
        memcpy(data_, begin, n);
        size_ = n;
        Traits::assign(data_[n], T());
    }
}

template <class Iterator, class T>
inline void replace(Iterator first, Iterator last, const T& old_value, const T& new_value)
{
    for (; first != last; ++first) {
        if (*first == old_value) {
            *first = new_value;
        }
    }
}
}

typedef std::basic_string<char, std::char_traits<char>, mwFileMemAllocator<char, 3> > mwFileString;

template <class H>
class mwFileXPHandle : public mwFile {
public:
    mwFileXPHandle(mwFileMountPoint&, unsigned long);
    virtual ~mwFileXPHandle();
    virtual unsigned long getReadAlign() = 0;
    virtual unsigned long getFileAlign() = 0;

    static void setBufferSize(unsigned long size);
    static unsigned long getBufferSize();
    bool lockBuffer(mwFileBuffer*& locked_buffer);
    void unlockBuffer();
    int close();
    int open(const char* path);

protected:
    mwFileBuffer buffer;
    bool buffer_locked;

private:
    static unsigned long sBufferSize;
};

template <class H>
unsigned long mwFileXPHandle<H>::sBufferSize = 0x8000;

template <class H>
class mwFileXPOpenCommand : public mwFileXPCommand<mwFileXPOpenCommand<H>, H> {
public:
    mwFileXPOpenCommand(mwFileCallback completion_callback, void* completion_data,
                        const mwFileString& path, unsigned long flags,
                        mwFileMountPoint& mount_point);
    virtual ~mwFileXPOpenCommand();
    virtual mwFileAsyncResult getResult() const;

    int handleNotStartedState();

    int serviceState(int state)
    {
        switch (state) {
        case 0:
            return handleNotStartedState();
        }
        return mwFileCommandPlugin<mwFileXPOpenCommand<H> >::serviceState(state);
    }

private:
    mwFileString path;
};

template <class T, class H>
class mwFileXPCloseCommand : public mwFileXPCommand<T, H> {
public:
    mwFileXPCloseCommand(mwFileCallback completion_callback, void* completion_data,
                         mwFile* owning_file);
    virtual ~mwFileXPCloseCommand();
    virtual int abort();
    virtual mwFileAsyncResult getResult() const;

    int handleNotStartedState();
    int handleFlushingState();

    int serviceState(int state)
    {
        switch (state) {
        case 6:
            return handleFlushingState();
        case 0:
            return handleNotStartedState();
        }
        return mwFileCommandPlugin<T>::serviceState(state);
    }

protected:
    mwFileBuffer* buffer;
};

template <class T, class H>
class mwFileXPReadUnbufferedCommand : public mwFileXPCommand<T, H>,
                                      public mwFileCommandProgress {
public:
    mwFileXPReadUnbufferedCommand(mwFileCallback completion_callback,
                                  void* completion_data, mwFile* owning_file,
                                  unsigned long long offset, void* buffer,
                                  unsigned long length, unsigned char priority);
    virtual ~mwFileXPReadUnbufferedCommand();
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
    virtual int abort();
    virtual mwFileAsyncResult getResult() const;
    virtual int getProgressBytes(unsigned long&);

    int handleSignalState();
    int handleReadingState();
    int handleNotStartedState();

    int serviceState(int state)
    {
        switch (state) {
        case 7:
            return handleReadingState();
        case 0:
            return handleNotStartedState();
        case 2:
            return handleSignalState();
        }
        return mwFileCommandPlugin<T>::serviceState(state);
    }

protected:
    void* buffer;
    unsigned long long offset;
    unsigned long long end_offset;
    unsigned long length;
    unsigned long bytes_read;
    unsigned char priority;
};

template <class T, class H>
class mwFileXPReadBufferedCommand : public mwFileXPCommand<T, H>,
                                    public mwFileCommandProgress {
public:
    mwFileXPReadBufferedCommand(mwFileCallback completion_callback,
                                void* completion_data, mwFile* owning_file,
                                unsigned long long offset, void* buffer,
                                unsigned long length, unsigned char priority);
    virtual ~mwFileXPReadBufferedCommand();
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
    virtual int abort();
    virtual mwFileAsyncResult getResult() const;
    virtual int getProgressBytes(unsigned long&);

    int handleSignalState();
    int handleReadingState();
    int handleFillingState();
    int handleNotStartedState();

    int serviceState(int state)
    {
        switch (state) {
        case 7:
            return handleReadingState();
        case 8:
            return handleFillingState();
        case 0:
            return handleNotStartedState();
        case 2:
            return handleSignalState();
        }
        return mwFileCommandPlugin<T>::serviceState(state);
    }

protected:
    unsigned char* current_buffer;
    unsigned char* start_buffer;
    unsigned long long current_offset;
    unsigned long long start_offset;
    unsigned long start_length;
    unsigned long current_length;
    unsigned long bytes_read;
    mwFileBuffer* read_buffer;
    unsigned char priority;
};

template <class Command>
class mwFileGCIOMixIn : public mwFileGCErrorState {
public:
    mwFileGCIOMixIn();
    virtual void queryErrorState();

    int nativeAbort();
    unsigned long nativeProgress() const;
    int nativeReadAsync(void* destination, unsigned long length,
                        unsigned long long position, unsigned char priority);
    bool nativeContinueRead(mwFileError& result);
    int nativeWriteAsync(const void* source, unsigned long length,
                         unsigned long long position, unsigned char priority);
    bool nativeContinueWrite(mwFileError& result);
    static void gcCallbackThunk(long result, DVDFileInfo* info);

private:
    unsigned char* buffer;
    unsigned long long position;
    unsigned long remaining;
    unsigned long chunk;
    unsigned long progress;
    unsigned char priority;
    DVDFileInfo* file_info;
    int* open_count;
    volatile mwFileError error;
    long callback_result;
    bool busy;
    bool error_reported;
};

class mwFileGCHandle : public mwFileXPHandle<mwFileGCHandle> {
public:
    mwFileGCHandle(mwFileMountPoint&, unsigned long);
    virtual ~mwFileGCHandle();
    virtual int startCloseFileCommand(mwFileCommand*&, mwFileCallback, void*);
    virtual int startReadFileCommand(mwFileCommand*&, unsigned long long, void*,
                                     unsigned long, unsigned char, mwFileCallback,
                                     void*);
    virtual int startWriteFileCommand(mwFileCommand*&, unsigned long long, const void*,
                                      unsigned long, unsigned char, mwFileCallback,
                                      void*);
    virtual int startFlushFileCommand(mwFileCommand*&, mwFileCallback, void*);
    virtual unsigned long getReadAlign();
    virtual unsigned long getFileAlign();

    int nativeOpen(const char* path, unsigned long open_flags);
    DVDFileInfo* handle(unsigned char priority, int** open_count);

    bool openInstance(unsigned char index)
    {
        if (open_counts[index] != 0) {
            return true;
        }
        if (DVDFastOpen(entry_number, &file_info[index])) {
            ++open_counts[index];
            return true;
        }
        return false;
    }

    int nativeClose()
    {
        for (unsigned short i = 0; i < 3; i++) {
            if (open_counts[i] != 0) {
                DVDClose(&file_info[i]);
                --open_counts[i];
            }
        }
        return 0;
    }

private:
    long entry_number;
    unsigned long field_0x4c;
    DVDFileInfo file_info[3];
    unsigned long field_0x104;
    int open_counts[3];
};

class mwFileGCCloseCommand : public mwFileXPCloseCommand<mwFileGCCloseCommand, mwFileGCHandle>,
                             public mwFileGCIOMixIn<mwFileGCCloseCommand> {
public:
    mwFileGCCloseCommand(mwFileCallback completion_callback, void* completion_data,
                         mwFile* owning_file)
        : mwFileXPCloseCommand<mwFileGCCloseCommand, mwFileGCHandle>(
              completion_callback, completion_data, owning_file)
    {
    }

    virtual ~mwFileGCCloseCommand() {}
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileGCReadUnbufferedCommand
    : public mwFileXPReadUnbufferedCommand<mwFileGCReadUnbufferedCommand, mwFileGCHandle>,
      public mwFileGCIOMixIn<mwFileGCReadUnbufferedCommand> {
public:
    mwFileGCReadUnbufferedCommand(mwFileCallback completion_callback, void* completion_data,
                                  mwFile* owning_file, unsigned long long offset,
                                  void* buffer, unsigned long length, unsigned char priority)
        : mwFileXPReadUnbufferedCommand<mwFileGCReadUnbufferedCommand, mwFileGCHandle>(
              completion_callback, completion_data, owning_file, offset, buffer, length,
              priority)
    {
    }

    virtual ~mwFileGCReadUnbufferedCommand() {}
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileGCReadBufferedCommand
    : public mwFileXPReadBufferedCommand<mwFileGCReadBufferedCommand, mwFileGCHandle>,
      public mwFileGCIOMixIn<mwFileGCReadBufferedCommand> {
public:
    mwFileGCReadBufferedCommand(mwFileCallback completion_callback, void* completion_data,
                                mwFile* owning_file, unsigned long long offset,
                                void* buffer, unsigned long length, unsigned char priority)
        : mwFileXPReadBufferedCommand<mwFileGCReadBufferedCommand, mwFileGCHandle>(
              completion_callback, completion_data, owning_file, offset, buffer, length,
              priority)
    {
    }

    virtual ~mwFileGCReadBufferedCommand() {}
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileGCServer : public mwFileServerQueued {
public:
    void queryErrorState();
};

class gcnDriver {
public:
    gcnDriver() : service_stack(0), wakeup_stack(0), initialized(false) {}
    ~gcnDriver();

    static void* serviceThreadThunk(void*);
    static void* wakeupThreadThunk(void*);
    static void unlockAndUnmount(const char* mount);

    int initialize(_mwFileInitParam* param);
    void shutdown();
    void queryErrorState();

    mwFileGCServer server;
    OSThread service_thread;
    unsigned char* service_stack;
    OSThread wakeup_thread;
    unsigned char* wakeup_stack;
    OSMessageQueue wakeup_queue;
    OSMessage wakeup_messages[32];
    bool initialized;
};

extern gcnDriver& gcnGetDriver();

template <class T, class OpenCommand>
class mwFileMountPointPlugin : public mwFileMountPoint {
public:
    mwFileMountPointPlugin(const char* mount_name, mwFileServer& server_ref);
    virtual ~mwFileMountPointPlugin();
    virtual int startOpenFileCommand(mwFileCommand*&, const char*, unsigned long,
                                     mwFileCallback, void*);
    virtual int convertToPlatformPath(char*, const char*, unsigned long, unsigned long&);
    virtual const char* getInternalPath() const;
    virtual mwFileServer& getServer();

protected:
    OSMutex mutex;
    char internal_path[36];
    mwFileServer& server;
};

template <class T, class OpenCommand>
class mwFileDevicePlugin : public mwFileMountPointPlugin<T, OpenCommand>, public mwFileDevice {
public:
    mwFileDevicePlugin(const char* mount_name, mwFileServer& server_ref);
    virtual ~mwFileDevicePlugin();
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
    virtual void setErrorCallback(mwFileDeviceErrorCallback, void*);
    virtual void reportError(mwFileError, mwFileCommand*);
    virtual void revokeError(mwFileCommand*);
};

template <class Traits>
class mwFileHandlePlugin : public mwFile {
public:
    mwFileHandlePlugin(mwFileMountPoint&, unsigned long);
    virtual ~mwFileHandlePlugin();
    virtual int startCloseFileCommand(mwFileCommand*&, mwFileCallback, void*);
    virtual int startReadFileCommand(mwFileCommand*&, unsigned long long, void*,
                                     unsigned long, unsigned char, mwFileCallback,
                                     void*);
    virtual int startWriteFileCommand(mwFileCommand*&, unsigned long long, const void*,
                                      unsigned long, unsigned char, mwFileCallback,
                                      void*);
    virtual int startFlushFileCommand(mwFileCommand*&, mwFileCallback, void*);
};

template <class T, class OpenCommand>
mwFileMountPointPlugin<T, OpenCommand>::~mwFileMountPointPlugin()
{
}

template <class T, class OpenCommand>
mwFileDevicePlugin<T, OpenCommand>::~mwFileDevicePlugin()
{
}

template <class Traits>
mwFileHandlePlugin<Traits>::~mwFileHandlePlugin()
{
}

namespace {
class gcnHostCloseCommand;
class gcnHostFlushCommand;
class gcnHostWriteCommand;
class gcnHostOpenCommand;
class gcnHostMountPoint;

struct gcnHostFileTraits {
    typedef gcnHostCloseCommand CloseCommand;
    typedef gcnHostFlushCommand FlushCommand;
    typedef gcnHostWriteCommand WriteCommand;
    typedef mwFileNotSupportedCommand ReadCommand;
};

class gcnHostFileHandle : public mwFileHandlePlugin<gcnHostFileTraits> {
public:
    gcnHostFileHandle(mwFileMountPoint& mount_point, unsigned long open_flags,
                      const mwFileString& path)
        : mwFileHandlePlugin<gcnHostFileTraits>(mount_point, open_flags), host_file(0)
    {
        setDebugName(path.data());
    }

    virtual ~gcnHostFileHandle();

    FILE* host_file;
};

class gcnHostCloseCommand : public mwFileCommandPlugin<gcnHostCloseCommand> {
public:
    gcnHostCloseCommand(mwFileCallback completion_callback, void* completion_data,
                        mwFile* owning_file)
        : mwFileCommandPlugin<gcnHostCloseCommand>(completion_callback, completion_data,
                                                   owning_file)
    {
    }

    virtual ~gcnHostCloseCommand();
    virtual mwFileAsyncResult getResult() const;
    int serviceState(int state);
};

class gcnHostFlushCommand : public mwFileCommandPlugin<gcnHostFlushCommand> {
public:
    gcnHostFlushCommand(mwFileCallback completion_callback, void* completion_data,
                        mwFile* owning_file)
        : mwFileCommandPlugin<gcnHostFlushCommand>(completion_callback, completion_data,
                                                   owning_file)
    {
    }

    virtual ~gcnHostFlushCommand();
    virtual mwFileAsyncResult getResult() const;
    int serviceState(int state);
};

class gcnHostWriteCommand : public mwFileCommandPlugin<gcnHostWriteCommand> {
public:
    gcnHostWriteCommand(mwFileCallback completion_callback, void* completion_data,
                        mwFile* owning_file, unsigned long long offset_value,
                        const void* source_buffer, unsigned long write_length,
                        unsigned char write_priority)
        : mwFileCommandPlugin<gcnHostWriteCommand>(completion_callback, completion_data,
                                                   owning_file),
          source(source_buffer),
          offset(offset_value),
          length(write_length),
          priority(write_priority)
    {
    }

    virtual ~gcnHostWriteCommand();
    virtual mwFileAsyncResult getResult() const;
    int serviceState(int state);

private:
    const void* source;
    unsigned long long offset;
    unsigned long length;
    unsigned long bytes_written;
    unsigned char priority;
};

class gcnHostOpenCommand : public mwFileCommandPlugin<gcnHostOpenCommand> {
public:
    gcnHostOpenCommand(mwFileCallback completion_callback, void* completion_data,
                       const mwFileString& path, unsigned long open_flags,
                       gcnHostMountPoint& mount_point);
    virtual ~gcnHostOpenCommand();
    virtual mwFileAsyncResult getResult() const;
    int handleNotStartedState();
    int serviceState(int state);

private:
    mwFileString name;
};

class gcnHostMountPoint : public mwFileDevicePlugin<gcnHostMountPoint, gcnHostOpenCommand> {
public:
    gcnHostMountPoint(const char* mount_name, mwFileServer& server_ref);
    const char* convertPath(const char* path);

private:
    mwFileString full_path;
};

class gcnCDMountPoint
    : public mwFileDevicePlugin<gcnCDMountPoint, mwFileXPOpenCommand<mwFileGCHandle> > {
public:
    gcnCDMountPoint(const char* mount_name, mwFileServer& server_ref);
    const char* convertPath(const char* path);

private:
    mwFileString full_path;
};
}

template <class T>
T mwRoundUp(T value, unsigned int alignment);

static inline void gcnInterlockedIncrement(volatile int& value)
{
    int enabled = OSDisableInterrupts();
    ++value;
    OSRestoreInterrupts(enabled);
}

static inline void gcnInterlockedDecrement(volatile int& value)
{
    int enabled = OSDisableInterrupts();
    --value;
    OSRestoreInterrupts(enabled);
}

static inline mwFileError gcnConvertDVDStatus(long status)
{
    switch (status) {
    case DVD_STATE_END:
    case DVD_STATE_BUSY:
    case DVD_STATE_WAITING:
        return MWFILE_ERROR_NONE;
    case DVD_STATE_FATAL_ERROR:
        return MWFILE_ERROR_FATAL;
    case DVD_STATE_NO_DISK:
        return MWFILE_ERROR_NO_DISK;
    case DVD_STATE_COVER_OPEN:
        return MWFILE_ERROR_COVER_OPEN;
    case DVD_STATE_WRONG_DISK:
        return MWFILE_ERROR_WRONG_DISK;
    case DVD_STATE_CANCELED:
        return MWFILE_ERROR_ABORTED;
    case DVD_STATE_RETRY:
    default:
        return MWFILE_ERROR_DRIVE;
    }
}

void mwFileGCServer::queryErrorState()
{
    mwFileOSMutexLock lock(mutex);

    for (unsigned char i = 0; i <= 2; i++) {
        mwFileCommand* command = active[i];
        if (command != 0) {
            void* queried_object;
            mwFileGCErrorState* state;
            if (command->isA(&mwFileClassTypeInfo<mwFileGCErrorState>::sTypeInfo,
                             queried_object)) {
                state = static_cast<mwFileGCErrorState*>(queried_object);
            } else {
                state = 0;
            }
            if (state != 0) {
                state->queryErrorState();
            }
        }
    }
}

namespace {
const char* gcnCDMountPoint::convertPath(const char* path)
{
    full_path = &stringBase0[SLASH_STRING];
    full_path.append(path, strlen(path));
    std::replace(full_path.begin(), full_path.end(), '\\', '/');
    return full_path.data();
}

gcnCDMountPoint::gcnCDMountPoint(const char* mount_name, mwFileServer& server_ref)
    : mwFileDevicePlugin<gcnCDMountPoint, mwFileXPOpenCommand<mwFileGCHandle> >(mount_name,
                                                                               server_ref)
{
    full_path.reserve(0x200);
}

const char* gcnHostMountPoint::convertPath(const char* path)
{
    full_path = &stringBase0[SLASH_STRING];
    full_path.append(path, strlen(path));
    std::replace(full_path.begin(), full_path.end(), '\\', '/');
    return full_path.data();
}

gcnHostMountPoint::gcnHostMountPoint(const char* mount_name, mwFileServer& server_ref)
    : mwFileDevicePlugin<gcnHostMountPoint, gcnHostOpenCommand>(mount_name, server_ref)
{
    full_path.reserve(0x200);
}

gcnHostFileHandle::~gcnHostFileHandle()
{
}

mwFileAsyncResult gcnHostCloseCommand::getResult() const
{
    mwFileAsyncResult result;
    result.value.bytes = error;
    result.error = error;
    return result;
}

gcnHostCloseCommand::~gcnHostCloseCommand()
{
    delete file;
    file = 0;
}

gcnHostFlushCommand::~gcnHostFlushCommand()
{
}

mwFileAsyncResult gcnHostFlushCommand::getResult() const
{
    mwFileAsyncResult result;
    result.error = error;
    result.value.bytes = error;
    return result;
}

gcnHostWriteCommand::~gcnHostWriteCommand()
{
}

mwFileAsyncResult gcnHostWriteCommand::getResult() const
{
    mwFileAsyncResult result;
    result.error = error;
    result.value.bytes = bytes_written;
    return result;
}

#pragma push
#pragma opt_common_subs off
mwFileAsyncResult gcnHostOpenCommand::getResult() const
{
    mwFileAsyncResult result;
    result.value.file = error == 0 ? file : 0;
    result.error = error;
    return result;
}
#pragma pop

/* TODO: [Scope warn] Write requests require append or truncate mode, as supplied by mwFileOpenModeToFlags. */
static inline int openHostFile(gcnHostFileHandle* handle, const char* path)
{
    unsigned long open_flags = handle->getFlags();
    char type;
    char plus = 0;

    if ((open_flags & 2) != 0) {
        if ((open_flags & 4) != 0) {
            type = 'a';
            if ((open_flags & 1) != 0) {
                plus = '+';
            }
        } else if ((open_flags & 8) != 0) {
            type = 'w';
            if ((open_flags & 1) != 0) {
                plus = '+';
            }
        }
    } else {
        _mwFileNoOp(&stringBase0[HOST_READ_MESSAGE], path);
        return -13;
    }

    char mode[4];
    mode[0] = type;
    mode[1] = 'b';
    mode[2] = plus;
    mode[3] = '\0';
    handle->host_file = fopen(path, mode);
    if (handle->host_file == 0) {
        return -18;
    } else {
        long position = ftell(handle->host_file);
        fseek(handle->host_file, 0, 2);
        handle->setSize(ftell(handle->host_file));
        fseek(handle->host_file, position, 0);
        return 0;
    }
}

int gcnHostOpenCommand::handleNotStartedState()
{
    if (file == 0) {
        error = -10;
        return 2;
    }

    const char* path = name.data();
    error = openHostFile(static_cast<gcnHostFileHandle*>(file), path);
    return 2;
}

gcnHostOpenCommand::~gcnHostOpenCommand()
{
    if (error != 0) {
        delete file;
        file = 0;
    }
}

gcnHostOpenCommand::gcnHostOpenCommand(mwFileCallback completion_callback,
                                       void* completion_data, const mwFileString& path,
                                       unsigned long open_flags,
                                       gcnHostMountPoint& mount_point)
    : mwFileCommandPlugin<gcnHostOpenCommand>(completion_callback, completion_data, 0),
      name(path)
{
    gcnHostFileHandle* handle = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[HOST_HANDLE_NAME])
        gcnHostFileHandle(mount_point, open_flags, path);
    file = handle;
}
}

void* gcnDriver::serviceThreadThunk(void*)
{
    _mwFileServiceThread();
    return 0;
}

void* gcnDriver::wakeupThreadThunk(void* argument)
{
    gcnDriver* driver = static_cast<gcnDriver*>(argument);
    mwFileCommand* command;

    for (;;) {
        OSReceiveMessage(&driver->wakeup_queue,
                         reinterpret_cast<OSMessage*>(&command), 1);
        if (command == 0) {
            break;
        }
        command->wakeup();
    }

    return 0;
}

void gcnDriver::unlockAndUnmount(const char* mount)
{
    mwFileMountPoint* mount_point;

    if (mwFileMountTable::get().getMountPointFromName(mount_point, mount) == 0) {
        gcnInterlockedDecrement(mount_point->referenceCount());
        mwFileUnmount(mount);
    }
}

void gcnDriver::shutdown()
{
    if (initialized) {
        OSJoinThread(&service_thread, 0);
        unlockAndUnmount(&stringBase0[HOST_MOUNT_NAME]);
        unlockAndUnmount(&stringBase0[CDROM_MOUNT_NAME]);
        mwFileUnregisterFileServer(server);
        if (server.isInitialized()) {
            server.shutdown();
        }
        OSJamMessage(&wakeup_queue, 0, 1);
        OSJoinThread(&wakeup_thread, 0);
        initialized = false;
    }
}

int gcnDriver::initialize(_mwFileInitParam* param)
{
    if (initialized) {
        return 0;
    }

    OSInitMessageQueue(&wakeup_queue, wakeup_messages, 32);

    if (wakeup_stack == 0) {
        if (!mwFileMemAlloc(wakeup_stack, 0x800, MW_TARGET_MEM_ALIGN_BUFFER,
                            &stringBase0[WAKEUP_STACK_NAME])) {
            _mwFileNoOp(&stringBase0[WAKEUP_STACK_MESSAGE]);
            return -10;
        }
    }
    if (!OSCreateThread(&wakeup_thread, wakeupThreadThunk, this, wakeup_stack + 0x800, 0x800, 9,
                        1)) {
        _mwFileNoOp(&stringBase0[WAKEUP_THREAD_MESSAGE]);
        return -1;
    }
    OSResumeThread(&wakeup_thread);

    if (service_stack == 0) {
        if (!mwFileMemAlloc(service_stack, 0x2000, MW_TARGET_MEM_ALIGN_BUFFER,
                            &stringBase0[SERVICE_STACK_NAME])) {
            _mwFileNoOp(&stringBase0[SERVICE_STACK_MESSAGE]);
            return -10;
        }
    }
    if (!OSCreateThread(&service_thread, serviceThreadThunk, 0, service_stack + 0x2000, 0x2000,
                        10, 1)) {
        _mwFileNoOp(&stringBase0[SERVICE_THREAD_MESSAGE]);
        return -1;
    }
    OSResumeThread(&service_thread);

    if (!server.isInitialized()) {
        int error = server.initialize(param->max_commands);
        if (error != 0) {
            return error;
        }
        error = mwFileRegisterFileServer(server);
        if (error != 0) {
            return error;
        }
    }

    gcnCDMountPoint* cd_mount = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[CD_MOUNT_POINT_NAME])
        gcnCDMountPoint(&stringBase0[CDROM_MOUNT_NAME], server);
    if (cd_mount == 0) {
        return -10;
    }
    int error = mwFileMountTable::get().addMountPoint(cd_mount);
    if (error != 0) {
        delete cd_mount;
        return error;
    }
    gcnInterlockedIncrement(cd_mount->referenceCount());

    gcnHostMountPoint* host_mount =
        new (MW_TARGET_MEM_ALIGN_32, &stringBase0[HOST_MOUNT_POINT_NAME])
            gcnHostMountPoint(&stringBase0[HOST_MOUNT_NAME], server);
    if (host_mount == 0) {
        return -10;
    }
    error = mwFileMountTable::get().addMountPoint(host_mount);
    if (error != 0) {
        delete host_mount;
        return error;
    }
    gcnInterlockedIncrement(host_mount->referenceCount());

    initialized = true;
    return 0;
}

gcnDriver::~gcnDriver()
{
    shutdown();
    if (wakeup_stack != 0) {
        mwFileMultithreadedMemTraits::deallocate(wakeup_stack);
        wakeup_stack = 0;
    }
    if (service_stack != 0) {
        mwFileMultithreadedMemTraits::deallocate(service_stack);
        service_stack = 0;
    }
}

int mwFileGCHandle::startFlushFileCommand(mwFileCommand*& command,
                                          mwFileCallback completion_callback,
                                          void* completion_data)
{
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[FLUSH_COMMAND_NAME])
        mwFileNotSupportedCommand(completion_callback, completion_data, this);
    return addCommand(command, 1);
}

int mwFileGCHandle::startWriteFileCommand(mwFileCommand*& command, unsigned long long offset,
                                          const void* buffer, unsigned long length,
                                          unsigned char priority,
                                          mwFileCallback completion_callback,
                                          void* completion_data)
{
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[WRITE_COMMAND_NAME])
        mwFileNotSupportedCommand(completion_callback, completion_data, this);
    return addCommand(command, priority);
}

int mwFileGCHandle::startReadFileCommand(mwFileCommand*& command, unsigned long long offset,
                                         void* buffer, unsigned long length,
                                         unsigned char priority,
                                         mwFileCallback completion_callback,
                                         void* completion_data)
{
    bool aligned = false;

    if (offset == mwRoundUp(offset, 4) && length == mwRoundUp(length, 32) &&
        (unsigned long)buffer == mwRoundUp((unsigned long)buffer, 32)) {
        aligned = true;
    }
    bool unbuffered = aligned;
    if (aligned && (unsigned long)buffer >= 0x7E000000 && (unsigned long)buffer < 0x80000000) {
        unbuffered = false;
    }

    if (unbuffered) {
        command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[UNBUFFERED_READ_NAME])
            mwFileGCReadUnbufferedCommand(completion_callback, completion_data, this, offset,
                                          buffer, length, priority);
    } else {
        _mwFileNoOp(&stringBase0[BUFFERED_READ_MESSAGE], getDebugName(), (int)offset, length);
        command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[BUFFERED_READ_NAME])
            mwFileGCReadBufferedCommand(completion_callback, completion_data, this, offset,
                                        buffer, length, priority);
    }
    return addCommand(command, priority);
}

int mwFileGCHandle::startCloseFileCommand(mwFileCommand*& command,
                                          mwFileCallback completion_callback,
                                          void* completion_data)
{
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[CLOSE_COMMAND_NAME])
        mwFileGCCloseCommand(completion_callback, completion_data, this);
    return addCommand(command, 1);
}

DVDFileInfo* mwFileGCHandle::handle(unsigned char priority, int** open_count)
{
    if (open_counts[priority & 3] == 0) {
        if (!openInstance(priority & 3)) {
            return 0;
        }
    }
    if (open_count != 0) {
        *open_count = &open_counts[priority & 3];
    }
    return &file_info[priority & 3];
}

int mwFileGCHandle::nativeOpen(const char* path, unsigned long open_flags)
{
    if ((open_flags & 2) != 0) {
        _mwFileNoOp(&stringBase0[DVD_WRITE_MESSAGE], path);
        return -24;
    }

    entry_number = DVDConvertPathToEntrynum(path);
    if (entry_number == -1) {
        return -18;
    }
    if (openInstance(1)) {
        size = handle(1, 0)->length;
        field_0x18 = handle(1, 0)->start_address >> 11;
        return 0;
    }
    return -18;
}

unsigned long mwFileGCHandle::getFileAlign()
{
    return 4;
}

unsigned long mwFileGCHandle::getReadAlign()
{
    return 32;
}

mwFileGCHandle::~mwFileGCHandle()
{
    OSGetCurrentThread();
}

mwFileGCHandle::mwFileGCHandle(mwFileMountPoint& mount_point, unsigned long open_flags)
    : mwFileXPHandle<mwFileGCHandle>(mount_point, open_flags), entry_number(-1)
{
    memset(file_info, 0, sizeof(file_info));
    memset(open_counts, 0, sizeof(open_counts));
}

void mwFileCondition::wait(mwFileMutex& mutex)
{
    OSWaitCond(this, &mutex);
}

void mwFileCondition::signal()
{
    OSSignalCond(this);
}

mwFileCondition::~mwFileCondition()
{
}

mwFileCondition::mwFileCondition()
{
    OSInitCond(this);
}

void mwFileEvent::reset()
{
    OSLockMutex(&mutex);
    signaled = false;
    OSUnlockMutex(&mutex);
}

void mwFileEvent::set()
{
    OSLockMutex(&mutex);
    signaled = true;
    OSUnlockMutex(&mutex);
    OSSignalCond(&condition);
}

mwFileEvent::mwFileEvent() : signaled(false)
{
    OSInitMutex(&mutex);
    OSInitCond(&condition);
}

void mwFileMutex::unlock()
{
    OSUnlockMutex(this);
}

void mwFileMutex::lock()
{
    OSLockMutex(this);
}

mwFileMutex::~mwFileMutex()
{
}

mwFileMutex::mwFileMutex()
{
    OSInitMutex(this);
}

int _mwFilePlatformInit(_mwFileInitParam* param)
{
    mwFileXPHandle<mwFileGCHandle>::setBufferSize(param->file_handle_tracking_size);
    if (!gcnGetDriver().server.isInitialized()) {
        int error = gcnGetDriver().initialize(param);
        if (error != 0) {
            return error;
        }
    }
    return 0;
}

bool _mwFilePlatformIsInServiceThread()
{
    gcnDriver& driver = gcnGetDriver();
    return OSGetCurrentThread() == &driver.service_thread;
}

int _mwFilePlatformInterlockedDecrement(volatile int& value)
{
    int interrupts = OSDisableInterrupts();
    int result = --value;
    OSRestoreInterrupts(interrupts);
    return result;
}

int _mwFilePlatformInterlockedIncrement(volatile int& value)
{
    int interrupts = OSDisableInterrupts();
    int result = ++value;
    OSRestoreInterrupts(interrupts);
    return result;
}

void _mwFilePlatformYieldTimeSlice()
{
    OSYieldThread();
}

int mwFileStringCompareIgnoreCase(const char* left, const char* right)
{
    while (*left != '\0' && *right != '\0') {
        int left_char = std::tolower(*left++);
        int right_char = std::tolower(*right++);

        if (left_char != right_char) {
            return left_char < right_char ? -1 : 1;
        }
    }

    if (*left == '\0' && *right == '\0') {
        return 0;
    }
    if (*left == '\0') {
        return -1;
    }
    return 1;
}

void _mwFilePlatformTick()
{
    gcnGetDriver().queryErrorState();
}

gcnDriver& gcnGetDriver()
{
    static gcnDriver sDriver;
    return sDriver;
}

namespace {
int gcnHostOpenCommand::serviceState(int state)
{
    switch (state) {
    case 0:
        return handleNotStartedState();
    }
    return mwFileCommandPlugin<gcnHostOpenCommand>::serviceState(state);
}

int gcnHostWriteCommand::serviceState(int state)
{
    switch (state) {
    case 0: {
        gcnHostFileHandle* handle = static_cast<gcnHostFileHandle*>(file);
        if (offset != (unsigned long)ftell(handle->host_file)) {
            unsigned long seek_result = fseek(handle->host_file,
                                              (long)(offset & 0x7FFFFFFF), 0);
            if (seek_result != (unsigned long)(offset & 0x7FFFFFFF)) {
                error = -9;
                return 2;
            }
        }
        bytes_written = fwrite(source, 1, length, handle->host_file);
        if (offset + bytes_written > handle->getSize()) {
            handle->updateSize(offset + bytes_written);
        }
        error = 0;
        return 2;
    }
    }
    return mwFileCommandPlugin<gcnHostWriteCommand>::serviceState(state);
}

int gcnHostFlushCommand::serviceState(int state)
{
    switch (state) {
    case 0:
        fflush(static_cast<gcnHostFileHandle*>(file)->host_file);
        error = 0;
        break;
    default:
        return mwFileCommandPlugin<gcnHostFlushCommand>::serviceState(state);
    }
    return 2;
}

int gcnHostCloseCommand::serviceState(int state)
{
    switch (state) {
    case 0: {
        gcnHostFileHandle* handle = static_cast<gcnHostFileHandle*>(file);
        if (handle->host_file != 0) {
            fclose(handle->host_file);
            handle->host_file = 0;
        }
        error = 0;
        break;
    }
    default:
        return mwFileCommandPlugin<gcnHostCloseCommand>::serviceState(state);
    }
    return 2;
}
}

/* TODO: [near miss] 99.16667% for mwFileGCReadBufferedCommand; this/previous
 * saved homes swap; loop-local snapshot scope is neutral. */
template <class T>
int mwFileCommandPlugin<T>::service()
{
    int state = signal_state;
    int previous;
    bool finished;

    do {
        previous = state;
        state = static_cast<T*>(this)->serviceState(previous);
        finished = true;
        bool unchanged = state == 4 || state == previous;
        if (!unchanged && hasReferences()) {
            finished = false;
        }
    } while (!finished);

    signal_state = state;
    if (state == 4) {
        return 2;
    }
    return hasReferences();
}

template <class T, class OpenCommand>
mwFileMountPointPlugin<T, OpenCommand>::mwFileMountPointPlugin(const char* mount_name,
                                                               mwFileServer& server_ref)
    : mwFileMountPoint(mount_name),
      server((OSInitMutex(&mutex), server_ref))
{
    strcpy(internal_path, &stringBase0[SLASH_STRING]);
    strcat(internal_path, mount_name);
    strcat(internal_path, &stringBase0[SLASH_STRING]);
}

template <class T, class OpenCommand>
int mwFileMountPointPlugin<T, OpenCommand>::startOpenFileCommand(
    mwFileCommand*& command, const char* path, unsigned long open_flags,
    mwFileCallback completion_callback, void* completion_data)
{
    {
        mwFileOSMutexLock lock(mutex);
        const char* converted = static_cast<T*>(this)->convertPath(path);
        if (converted == 0) {
            return -15;
        }
        command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[OPEN_COMMAND_NAME])
            OpenCommand(completion_callback, completion_data, mwFileString(converted),
                        open_flags, *static_cast<T*>(this));
        if (command == 0) {
            return -10;
        }
    }

    int error = getServer().addCommand(command, 1);
    if (error != 0) {
        delete command;
        command = 0;
    }
    return error;
}

template <class T, class OpenCommand>
int mwFileMountPointPlugin<T, OpenCommand>::convertToPlatformPath(
    char* buffer, const char* path, unsigned long size, unsigned long& required)
{
    mwFileOSMutexLock lock(mutex);
    const char* converted = static_cast<T*>(this)->convertPath(path);
    if (converted == 0) {
        return -15;
    }
    required = strlen(converted) + 1;
    if (size >= required) {
        if (buffer != 0) {
            strcpy(buffer, converted);
        }
        return 0;
    }
    return -26;
}

template <class T, class OpenCommand>
const char* mwFileMountPointPlugin<T, OpenCommand>::getInternalPath() const
{
    return internal_path;
}

template <class T, class OpenCommand>
mwFileServer& mwFileMountPointPlugin<T, OpenCommand>::getServer()
{
    return server;
}

template <class T, class OpenCommand>
mwFileDevicePlugin<T, OpenCommand>::mwFileDevicePlugin(const char* mount_name,
                                                       mwFileServer& server_ref)
    : mwFileMountPointPlugin<T, OpenCommand>(mount_name, server_ref)
{
    error_callback = 0;
    error_callback_data = 0;
    error_count = -1;
}

template <class T, class OpenCommand>
unsigned char mwFileDevicePlugin<T, OpenCommand>::isA(mwFileTypeInfo* type,
                                                      const void*& object) const
{
    if (type == &mwFileClassTypeInfo<mwFileDevice>::sTypeInfo) {
        object = static_cast<const mwFileDevice*>(this);
        return true;
    }
    if (mwFileQueryable::isA(type, object)) {
        return true;
    }
    object = 0;
    return false;
}

template <class T, class OpenCommand>
void mwFileDevicePlugin<T, OpenCommand>::setErrorCallback(mwFileDeviceErrorCallback callback,
                                                          void* callback_data)
{
    error_callback = callback;
    error_callback_data = callback_data;
}

template <class T, class OpenCommand>
void mwFileDevicePlugin<T, OpenCommand>::reportError(mwFileError error, mwFileCommand* command)
{
    int enabled = OSDisableInterrupts();
    int count = ++error_count;
    OSRestoreInterrupts(enabled);
    if (count == 0 && error_callback != 0) {
        mwFileDevice::Callback callback;
        callback.function = error_callback;
        callback.callback_data = error_callback_data;
        callback.state = MWFILE_ERROR_STATE_RAISED;
        callback.error = error;
        callback.path = this->getName();
        callback.command = command;
        mwFileDevice::queueErrorCallback(callback);
    }
}

template <class T, class OpenCommand>
void mwFileDevicePlugin<T, OpenCommand>::revokeError(mwFileCommand* command)
{
    int enabled = OSDisableInterrupts();
    int count = --error_count;
    OSRestoreInterrupts(enabled);
    if (count == -1 && error_callback != 0) {
        mwFileDevice::Callback callback;
        callback.function = error_callback;
        callback.callback_data = error_callback_data;
        callback.state = MWFILE_ERROR_STATE_CLEARED;
        callback.error = MWFILE_ERROR_NONE;
        callback.path = this->getName();
        callback.command = command;
        mwFileDevice::queueErrorCallback(callback);
    }
}

template <class Traits>
mwFileHandlePlugin<Traits>::mwFileHandlePlugin(mwFileMountPoint& mount_point,
                                               unsigned long open_flags)
    : mwFile(mount_point, open_flags)
{
}

template <class Traits>
int mwFileHandlePlugin<Traits>::startCloseFileCommand(mwFileCommand*& command,
                                                      mwFileCallback completion_callback,
                                                      void* completion_data)
{
    typedef typename Traits::CloseCommand CloseCommand;
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[CLOSE_COMMAND_NAME])
        CloseCommand(completion_callback, completion_data, this);
    return addCommand(command, 1);
}

template <class Traits>
int mwFileHandlePlugin<Traits>::startWriteFileCommand(mwFileCommand*& command,
                                                      unsigned long long offset,
                                                      const void* buffer, unsigned long length,
                                                      unsigned char priority,
                                                      mwFileCallback completion_callback,
                                                      void* completion_data)
{
    typedef typename Traits::WriteCommand WriteCommand;
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[WRITE_COMMAND_NAME])
        WriteCommand(completion_callback, completion_data, this, offset, buffer, length,
                     priority);
    return addCommand(command, priority);
}

template <class Traits>
int mwFileHandlePlugin<Traits>::startFlushFileCommand(mwFileCommand*& command,
                                                      mwFileCallback completion_callback,
                                                      void* completion_data)
{
    typedef typename Traits::FlushCommand FlushCommand;
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[FLUSH_COMMAND_NAME])
        FlushCommand(completion_callback, completion_data, this);
    return addCommand(command, 1);
}

template <class Traits>
int mwFileHandlePlugin<Traits>::startReadFileCommand(mwFileCommand*& command,
                                                     unsigned long long offset, void* buffer,
                                                     unsigned long length,
                                                     unsigned char priority,
                                                     mwFileCallback completion_callback,
                                                     void* completion_data)
{
    typedef typename Traits::ReadCommand ReadCommand;
    command = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[READ_COMMAND_NAME])
        ReadCommand(completion_callback, completion_data, this);
    return addCommand(command, priority);
}

template <class H>
mwFileXPHandle<H>::mwFileXPHandle(mwFileMountPoint& mount_point, unsigned long open_flags)
    : mwFile(mount_point, open_flags), buffer_locked(false)
{
    buffer.initialize(getBufferSize(), MW_TARGET_MEM_ALIGN_BUFFER);
}

template <class H>
mwFileXPHandle<H>::~mwFileXPHandle()
{
    buffer.shutdown();
}

template <class H>
void mwFileXPHandle<H>::setBufferSize(unsigned long size)
{
    sBufferSize = size;
}

template <class H>
unsigned long mwFileXPHandle<H>::getBufferSize()
{
    return sBufferSize;
}

template <class H>
bool mwFileXPHandle<H>::lockBuffer(mwFileBuffer*& locked_buffer)
{
    if (buffer_locked) {
        return false;
    }
    buffer_locked = true;
    locked_buffer = &buffer;
    return true;
}

template <class H>
void mwFileXPHandle<H>::unlockBuffer()
{
    buffer_locked = false;
}

template <class H>
int mwFileXPHandle<H>::close()
{
    return static_cast<H*>(this)->nativeClose();
}

template <class H>
int mwFileXPHandle<H>::open(const char* path)
{
    if (buffer.getData() == 0) {
        return -10;
    }
    int error = static_cast<H*>(this)->nativeOpen(path, flags);
    if (error != 0) {
        return error;
    }
    setDebugName(path);
    if ((flags & 4) != 0) {
        position = size;
    } else {
        position = 0;
    }
    return 0;
}

template <class H>
mwFileXPOpenCommand<H>::mwFileXPOpenCommand(mwFileCallback completion_callback,
                                            void* completion_data, const mwFileString& path_value,
                                            unsigned long open_flags,
                                            mwFileMountPoint& mount_point)
    : mwFileXPCommand<mwFileXPOpenCommand<H>, H>(completion_callback, completion_data, 0),
      path(path_value)
{
    this->file = new (MW_TARGET_MEM_ALIGN_32, &stringBase0[PLATFORM_HANDLE_NAME])
        H(mount_point, open_flags);
}

template <class H>
mwFileXPOpenCommand<H>::~mwFileXPOpenCommand()
{
    if (this->file != 0 && this->error != 0) {
        delete this->file;
        this->file = 0;
    }
}

/* TODO: [breakthrough needed] 82.69231%; error reload/copy remain;
 * definition/body CSE scopes restore before deferred emission. */
template <class H>
mwFileAsyncResult mwFileXPOpenCommand<H>::getResult() const
{
    mwFileAsyncResult result;
    result.value.file = this->error != 0 ? 0 : this->file;
    result.error = this->error;
    return result;
}

template <class H>
int mwFileXPOpenCommand<H>::handleNotStartedState()
{
    if (!this->isAborted()) {
        H* handle = static_cast<H*>(this->file);
        if (handle == 0) {
            this->error = -10;
        } else {
            this->error = handle->open(path.c_str());
        }
    }
    return 2;
}

template <class T, class H>
mwFileXPCloseCommand<T, H>::mwFileXPCloseCommand(mwFileCallback completion_callback,
                                                 void* completion_data, mwFile* owning_file)
    : mwFileXPCommand<T, H>(completion_callback, completion_data, owning_file), buffer(0)
{
}

template <class T, class H>
mwFileXPCloseCommand<T, H>::~mwFileXPCloseCommand()
{
    if (this->error == 0) {
        delete this->file;
        this->file = 0;
    }
}

template <class T, class H>
int mwFileXPCloseCommand<T, H>::abort()
{
    int result = mwFileCommandPluginBase::abort();
    if (result == 0 && this->signal_state == 6) {
        result = static_cast<T*>(this)->nativeAbort();
    }
    return result;
}

template <class T, class H>
mwFileAsyncResult mwFileXPCloseCommand<T, H>::getResult() const
{
    mwFileAsyncResult result;
    result.value.bytes = this->error;
    result.error = this->error;
    return result;
}

/* TODO: [near miss] 99.62963%; six mask/data home rows remain;
 * recover padding ownership; canonical round-up stays out of line. */
template <class T, class H>
int mwFileXPCloseCommand<T, H>::handleNotStartedState()
{
    if (this->isAborted()) {
        this->error = -16;
        this->file = 0;
    } else {
        if ((static_cast<H*>(this->file)->getFlags() & 2) != 0) {
            if (!static_cast<H*>(this->file)->lockBuffer(buffer)) {
                this->error = -12;
                return 2;
            }
            if (buffer->getLength() != 0 && buffer->isDirty()) {
                unsigned long read_mask = static_cast<H*>(this->file)->getReadAlign() - 1;
                unsigned long used = buffer->getLength();
                unsigned long padded = (used + read_mask) & ~read_mask;
                if (padded != used) {
                    memset(buffer->getData() + used, 0, padded - used);
                }
                this->error = static_cast<T*>(this)->nativeWriteAsync(buffer->getData(), padded,
                                                                       buffer->getPosition(), 1);
                if (this->error != 0) {
                    return 2;
                }
                return 6;
            }
            static_cast<H*>(this->file)->unlockBuffer();
        }
        this->error = static_cast<H*>(this->file)->close();
    }
    return 2;
}

template <class T, class H>
int mwFileXPCloseCommand<T, H>::handleFlushingState()
{
    mwFileError result;

    if (static_cast<T*>(this)->nativeContinueWrite(result)) {
        return 6;
    }
    if (result != 0) {
        this->error = result;
        static_cast<H*>(this->file)->unlockBuffer();
        return 2;
    }
    buffer->reset(~0ULL, 0);
    static_cast<H*>(this->file)->unlockBuffer();
    buffer = 0;
    if (this->error == 0) {
        this->error = static_cast<H*>(this->file)->close();
    }
    return 2;
}

template <class T, class H>
mwFileXPReadUnbufferedCommand<T, H>::mwFileXPReadUnbufferedCommand(
    mwFileCallback completion_callback, void* completion_data, mwFile* owning_file,
    unsigned long long offset_value, void* destination, unsigned long read_length,
    unsigned char read_priority)
    : mwFileXPCommand<T, H>(completion_callback, completion_data, owning_file),
      buffer(destination),
      offset(offset_value),
      end_offset(offset_value + read_length),
      length(read_length),
      bytes_read(0),
      priority(read_priority)
{
}

template <class T, class H>
mwFileXPReadUnbufferedCommand<T, H>::~mwFileXPReadUnbufferedCommand()
{
}

template <class T, class H>
inline unsigned char mwFileXPReadUnbufferedCommand<T, H>::isA(mwFileTypeInfo* type,
                                                        const void*& object) const
{
    if (type == &mwFileClassTypeInfo<mwFileCommandProgress>::sTypeInfo) {
        object = static_cast<const mwFileCommandProgress*>(this);
        return true;
    }
    object = 0;
    return false;
}

template <class T, class H>
int mwFileXPReadUnbufferedCommand<T, H>::abort()
{
    int result = mwFileCommandPluginBase::abort();
    if (result == 0 && this->signal_state == 7) {
        result = static_cast<T*>(this)->nativeAbort();
    }
    return result;
}

template <class T, class H>
mwFileAsyncResult mwFileXPReadUnbufferedCommand<T, H>::getResult() const
{
    mwFileAsyncResult result;
    result.error = this->error;
    result.value.bytes = bytes_read;
    return result;
}

template <class T, class H>
int mwFileXPReadUnbufferedCommand<T, H>::getProgressBytes(unsigned long& bytes)
{
    if (this->signal_state != 0) {
        bytes = static_cast<T*>(this)->nativeProgress();
    } else {
        bytes = 0;
    }
    return 0;
}

template <class T, class H>
int mwFileXPReadUnbufferedCommand<T, H>::handleSignalState()
{
    return mwFileCommandPluginBase::handleSignalState();
}

/* TODO: [breakthrough needed] 96.63265%; recover per-consumer nativeProgress
 * expansion and mixin ownership while preserving exact getProgressBytes. */
template <class T, class H>
int mwFileXPReadUnbufferedCommand<T, H>::handleReadingState()
{
    mwFileError result;

    if (!static_cast<T*>(this)->nativeContinueRead(result)) {
        if (this->isAborted()) {
            bytes_read = 0;
            this->error = -16;
            return 2;
        }
        if (result != 0) {
            this->error = result;
            bytes_read = 0;
            return 2;
        }
        bytes_read = static_cast<T*>(this)->nativeProgress();
        return 2;
    }
    gcnInterlockedDecrement(this->reference_count);
    return 7;
}

template <class T, class H>
int mwFileXPReadUnbufferedCommand<T, H>::handleNotStartedState()
{
    bytes_read = 0;
    if (this->isAborted()) {
        this->error = -16;
        return 2;
    }
    this->error = static_cast<T*>(this)->nativeReadAsync(buffer, length, offset, priority);
    if (this->error != 0) {
        return 2;
    }
    return 7;
}

template <class T, class H>
mwFileXPReadBufferedCommand<T, H>::mwFileXPReadBufferedCommand(
    mwFileCallback completion_callback, void* completion_data, mwFile* owning_file,
    unsigned long long offset_value, void* destination, unsigned long read_length,
    unsigned char read_priority)
    : mwFileXPCommand<T, H>(completion_callback, completion_data, owning_file),
      current_buffer(static_cast<unsigned char*>(destination)),
      start_buffer(current_buffer),
      current_offset(offset_value),
      start_offset(offset_value),
      start_length(read_length),
      current_length(read_length),
      bytes_read(0),
      read_buffer(0),
      priority(read_priority)
{
}

template <class T, class H>
mwFileXPReadBufferedCommand<T, H>::~mwFileXPReadBufferedCommand()
{
}

template <class T, class H>
inline unsigned char mwFileXPReadBufferedCommand<T, H>::isA(mwFileTypeInfo* type,
                                                      const void*& object) const
{
    if (type == &mwFileClassTypeInfo<mwFileCommandProgress>::sTypeInfo) {
        object = static_cast<const mwFileCommandProgress*>(this);
        return true;
    }
    object = 0;
    return false;
}

template <class T, class H>
int mwFileXPReadBufferedCommand<T, H>::abort()
{
    int result = mwFileCommandPluginBase::abort();
    if (result == 0 && this->signal_state == 3) {
        result = static_cast<T*>(this)->nativeAbort();
    }
    return result;
}

template <class T, class H>
mwFileAsyncResult mwFileXPReadBufferedCommand<T, H>::getResult() const
{
    mwFileAsyncResult result;
    result.error = this->error;
    result.value.bytes = bytes_read;
    return result;
}

template <class T, class H>
int mwFileXPReadBufferedCommand<T, H>::getProgressBytes(unsigned long& bytes)
{
    if (this->signal_state != 0) {
        bytes = bytes_read;
    } else {
        bytes = 0;
    }
    return 0;
}

template <class T, class H>
int mwFileXPReadBufferedCommand<T, H>::handleSignalState()
{
    if (read_buffer != 0) {
        if (this->error != 0) {
            read_buffer->reset(~0ULL, 0);
        }
        static_cast<H*>(this->file)->unlockBuffer();
        read_buffer = 0;
    }
    return mwFileCommandPluginBase::handleSignalState();
}

/* TODO: [near miss] 97.98561%; +f4 receiver copy order and offset/fill word homes
 * differ; needs original cached-receiver and read-phase ownership. */
template <class T, class H>
int mwFileXPReadBufferedCommand<T, H>::handleReadingState()
{
    if (this->isAborted()) {
        bytes_read = 0;
        this->error = -16;
        return 2;
    }

    if (!(current_offset < static_cast<H*>(this->file)->getSize())) {
        this->error = -9;
        return 2;
    }

    bytes_read += read_buffer->readInRange(current_buffer, current_length, current_offset);
    if (current_length == 0) {
        this->error = 0;
        return 2;
    }

    unsigned long long aligned_offset;
    if ((current_offset & (static_cast<H*>(this->file)->getFileAlign() - 1)) != 0) {
        H* handle = static_cast<H*>(this->file);
        unsigned long file_align = handle->getFileAlign();
        aligned_offset =
            (current_offset + file_align - 1) & ~(unsigned long long)(file_align - 1);
        aligned_offset -= handle->getFileAlign();
    } else {
        aligned_offset = current_offset;
    }

    unsigned long fill_length = read_buffer->getCapacity();
    if (aligned_offset + fill_length > static_cast<H*>(this->file)->getSize()) {
        fill_length = static_cast<H*>(this->file)->getSize() - aligned_offset;
    }
    read_buffer->reset(aligned_offset, fill_length);

    unsigned long read_mask = static_cast<H*>(this->file)->getReadAlign() - 1;
    this->error = static_cast<T*>(this)->nativeReadAsync(
        read_buffer->getData(), (fill_length + read_mask) & ~read_mask, aligned_offset,
        priority);
    if (this->error != 0) {
        return 2;
    }
    return 8;
}

template <class T, class H>
int mwFileXPReadBufferedCommand<T, H>::handleFillingState()
{
    mwFileError result;

    if (!static_cast<T*>(this)->nativeContinueRead(result)) {
        if (this->isAborted()) {
            bytes_read = 0;
            this->error = -16;
            return 2;
        }
        if (result != 0) {
            this->error = result;
            bytes_read = 0;
            return 2;
        }
        return 7;
    }
    gcnInterlockedDecrement(this->reference_count);
    return 8;
}

template <class T, class H>
int mwFileXPReadBufferedCommand<T, H>::handleNotStartedState()
{
    bytes_read = 0;
    current_offset = start_offset;
    current_length = start_length;
    current_buffer = start_buffer;
    if (this->isAborted()) {
        this->error = -16;
        return 2;
    }
    return static_cast<H*>(this->file)->lockBuffer(read_buffer) ? 7 : 0;
}

unsigned char mwFileGCCloseCommand::isA(mwFileTypeInfo* type, const void*& object) const
{
    if (type == &mwFileClassTypeInfo<mwFileGCErrorState>::sTypeInfo) {
        object = static_cast<const mwFileGCErrorState*>(this);
        return true;
    }
    if (mwFileQueryable::isA(type, object)) {
        return true;
    }
    object = 0;
    return false;
}

unsigned char mwFileGCReadUnbufferedCommand::isA(mwFileTypeInfo* type,
                                                  const void*& object) const
{
    if (type == &mwFileClassTypeInfo<mwFileGCErrorState>::sTypeInfo) {
        object = static_cast<const mwFileGCErrorState*>(this);
        return true;
    }
    if (mwFileXPReadUnbufferedCommand<mwFileGCReadUnbufferedCommand, mwFileGCHandle>::isA(
            type, object)) {
        return true;
    }
    object = 0;
    return false;
}

unsigned char mwFileGCReadBufferedCommand::isA(mwFileTypeInfo* type, const void*& object) const
{
    if (type == &mwFileClassTypeInfo<mwFileGCErrorState>::sTypeInfo) {
        object = static_cast<const mwFileGCErrorState*>(this);
        return true;
    }
    if (mwFileXPReadBufferedCommand<mwFileGCReadBufferedCommand, mwFileGCHandle>::isA(
            type, object)) {
        return true;
    }
    object = 0;
    return false;
}

template <class Command>
mwFileGCIOMixIn<Command>::mwFileGCIOMixIn()
    : buffer(0),
      position(0),
      remaining(0),
      chunk(0),
      progress(0),
      priority(1),
      file_info(0),
      open_count(0),
      busy(false),
      error_reported(false)
{
}

template <class Command>
void mwFileGCIOMixIn<Command>::queryErrorState()
{
    DVDFileInfo* info = file_info;
    if (info != 0) {
        mwFileDevice* device;
        Command* command = static_cast<Command*>(this);
        mwFile* owner = command->getFile();
        if (owner != 0) {
            device = owner->getDevice();
        } else {
            device = 0;
        }
        if (device != 0) {
            mwFileError status = gcnConvertDVDStatus(DVDGetCommandBlockStatus(&info->cb));
            if (error_reported &&
                (status == MWFILE_ERROR_NONE || status == MWFILE_ERROR_ABORTED ||
                 status == MWFILE_ERROR_FATAL)) {
                device->revokeError(command);
                error_reported = false;
            } else if (!error_reported && status != MWFILE_ERROR_NONE) {
                device->reportError(status, command);
                error_reported = true;
            }
        }
    }
}

template <class Command>
int mwFileGCIOMixIn<Command>::nativeAbort()
{
    return 0;
}

template <class Command>
unsigned long mwFileGCIOMixIn<Command>::nativeProgress() const
{
    int enabled = OSDisableInterrupts();
    unsigned long result = progress;
    OSRestoreInterrupts(enabled);
    return result;
}

template <class Command>
int mwFileGCIOMixIn<Command>::nativeReadAsync(void* destination, unsigned long length,
                                              unsigned long long offset,
                                              unsigned char read_priority)
{
    mwFileError result;

    file_info = static_cast<mwFileGCHandle*>(static_cast<Command*>(this)->getFile())
                    ->handle(read_priority, &open_count);
    if (file_info == 0) {
        return -12;
    }
    file_info->cb.user_data = this;
    buffer = static_cast<unsigned char*>(destination);
    position = offset;
    remaining = length;
    error = MWFILE_ERROR_NONE;
    callback_result = 0;
    chunk = 0;
    priority = read_priority;
    busy = false;
    nativeContinueRead(result);
    return result;
}

template <class Command>
bool mwFileGCIOMixIn<Command>::nativeContinueRead(mwFileError& result)
{
    int enabled = OSDisableInterrupts();
    bool in_progress = busy;
    OSRestoreInterrupts(enabled);
    if (in_progress) {
        result = MWFILE_ERROR_NONE;
        return true;
    }

    if (error != MWFILE_ERROR_NONE) {
        result = error;
        queryErrorState();
        file_info->cb.user_data = 0;
        file_info = 0;
        return false;
    }

    if (remaining == 0) {
        result = MWFILE_ERROR_NONE;
        if (error_reported) {
            queryErrorState();
        }
        file_info->cb.user_data = 0;
        file_info = 0;
        return false;
    }

    if (static_cast<Command*>(this)->isAborted()) {
        result = MWFILE_ERROR_ABORTED;
        queryErrorState();
        file_info->cb.user_data = 0;
        file_info = 0;
        return false;
    }

    busy = true;
    unsigned long length = remaining;
    if (length > 0x20000) {
        length = 0x20000;
    }
    remaining -= length;
    chunk = length;
    ++*open_count;
    DVDGetCommandBlockStatus(&file_info->cb);
    if (!DVDReadAsyncPrio(file_info, buffer, length, (long)(position & 0x7FFFFFFF),
                          gcCallbackThunk, (priority & 3) + 1)) {
        if (error_reported) {
            queryErrorState();
        }
        file_info->cb.user_data = 0;
        file_info = 0;
        return gcnConvertDVDStatus(DVDGetDriveStatus());
    }
    result = MWFILE_ERROR_NONE;
    return true;
}

template <class Command>
int mwFileGCIOMixIn<Command>::nativeWriteAsync(const void*, unsigned long,
                                               unsigned long long, unsigned char)
{
    return -24;
}

template <class Command>
bool mwFileGCIOMixIn<Command>::nativeContinueWrite(mwFileError&)
{
    return false;
}

template <class Command>
void mwFileGCIOMixIn<Command>::gcCallbackThunk(long result, DVDFileInfo* info)
{
    mwFileGCIOMixIn* io = static_cast<mwFileGCIOMixIn*>(info->cb.user_data);

    if (result == -3) {
        io->error = MWFILE_ERROR_ABORTED;
        io->callback_result = 0;
    } else {
        long status = DVDGetCommandBlockStatus(&info->cb);
        if (status == 0) {
            DCInvalidateRange(io->buffer, io->chunk);
            io->callback_result = result;
            io->error = MWFILE_ERROR_NONE;
            io->progress += io->chunk;
            io->position += io->chunk;
            io->buffer += io->chunk;
        } else {
            io->callback_result = 0;
            io->error = gcnConvertDVDStatus(status);
        }
    }

    io->busy = false;
    --*io->open_count;
    Command* command = static_cast<Command*>(io);
    if (!OSSendMessage(&gcnGetDriver().wakeup_queue, command, 0)) {
        _mwFileNoOp(&stringBase0[WAKEUP_ERROR_MESSAGE]);
    }
}


template <unsigned long ResultValue, int ResultError>
mwFileFailCommand<ResultValue, ResultError>::~mwFileFailCommand()
{
}

template <unsigned long ResultValue, int ResultError>
unsigned char mwFileFailCommand<ResultValue, ResultError>::isCompleted(
    mwFileAsyncResult& result) const
{
    result.value.bytes = ResultValue;
    result.error = ResultError;
    return completed;
}

template <unsigned long ResultValue, int ResultError>
unsigned char mwFileFailCommand<ResultValue, ResultError>::isAborted()
    const
{
    return 0;
}

template <unsigned long ResultValue, int ResultError>
mwFileAsyncResult
mwFileFailCommand<ResultValue, ResultError>::waitForCompletion() const
{
    mwFileAsyncResult result;
    while (!isCompleted(result)) {
        mwFileTick();
    }
    return result;
}

template <unsigned long ResultValue, int ResultError>
int mwFileFailCommand<ResultValue, ResultError>::abort()
{
    return 0;
}

template <unsigned long ResultValue, int ResultError>
int mwFileFailCommand<ResultValue, ResultError>::service()
{
    return 2;
}

template <unsigned long ResultValue, int ResultError>
void mwFileFailCommand<ResultValue, ResultError>::serviceCallback()
{
    completed = true;
    handleCompletion(waitForCompletion());
}

template <unsigned long ResultValue, int ResultError>
int mwFileFailCommand<ResultValue, ResultError>::close()
{
    return 0;
}

template <unsigned long ResultValue, int ResultError>
void mwFileFailCommand<ResultValue, ResultError>::deleteSelf()
{
    if (!completed || callback_depth != 0) {
        pending_delete = 1;
    } else {
        close();
        delete this;
    }
}

template class mwFileFailCommand<0, -13>;

void gcnDriver::queryErrorState()
{
    server.queryErrorState();
}

template <class T>
T mwRoundUp(T value, unsigned int alignment)
{
    return (value + alignment - 1) & ~static_cast<T>(alignment - 1);
}

template unsigned long mwRoundUp<unsigned long>(unsigned long, unsigned int);
template unsigned long long mwRoundUp<unsigned long long>(unsigned long long, unsigned int);
