#include "runtime/cstring.h"
#include "mw/mwFile.h"
#include "mw/mwFileContainers.h"
#include "mw/mwFileMutex.h"

extern int mwFileStringCompareIgnoreCase(const char*, const char*);

class mwFileServer;
struct mwFileTypeInfo;

class mwFileMultithreadedMemTraits {
public:
    static void deallocate(void* object);
};

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileMountPoint : public mwFileQueryable {
public:
    mwFileMountPoint(const char* name);
    mwFileMountPoint(const char* first, const char* last)
        : mount_count(0)
    {
        unsigned long length = last - first;
        memcpy(name, first, length);
        name[length] = '\0';
    }
    virtual ~mwFileMountPoint();
    virtual int startOpenFileCommand(mwFileCommand*&, const char*,
                                     unsigned long, mwFileCallback, void*) = 0;
    virtual int convertToPlatformPath(char*, const char*, unsigned long,
                                      unsigned long&) = 0;
    virtual const char* getInternalPath() const = 0;
    virtual mwFileServer* getServer() = 0;

    static void operator delete(void* object)
    {
        if (object != 0) {
            mwFileMultithreadedMemTraits::deallocate(object);
        }
    }

protected:
    friend class mwFileMountTable;
    char name[36];
    unsigned long mount_count;
};

extern mwFileServer* mwFileGetGenericCommandServer();

namespace {
class mwFileDummyMountPoint : public mwFileMountPoint {
public:
    mwFileDummyMountPoint(const char* first, const char* last);
    virtual ~mwFileDummyMountPoint();
    virtual int startOpenFileCommand(mwFileCommand*&, const char*,
                                     unsigned long, mwFileCallback, void*);
    virtual int convertToPlatformPath(char*, const char*, unsigned long,
                                      unsigned long&);
    virtual const char* getInternalPath() const;
    virtual mwFileServer* getServer();
};

const char* mwFileDummyMountPoint::getInternalPath() const
{
    return "";
}

int mwFileDummyMountPoint::convertToPlatformPath(
    char*, const char*, unsigned long, unsigned long&)
{
    return -13;
}

int mwFileDummyMountPoint::startOpenFileCommand(
    mwFileCommand*&, const char*, unsigned long, mwFileCallback, void*)
{
    return -13;
}

mwFileDummyMountPoint::mwFileDummyMountPoint(const char* first,
                                             const char* last)
    : mwFileMountPoint(first, last)
{
}
}

class mwFileCondition : public OSCond {
public:
    mwFileCondition();
    ~mwFileCondition();
    void signal();
    void wait(mwFileMutex&);
};

template <class T, class Allocator>
class circular_buffer {
private:
    std::vector<T, Allocator> values;
    unsigned long read_index;
    unsigned long write_index;
    unsigned long count;
};

template <class T>
class mwProducerConsumerQueue {
public:
    unsigned char consumeNonBlocking(T& value);
    void produce(T value);
    void resize(unsigned long size);

private:
    mwFileMutex mutex;
    mwFileCondition condition;
    circular_buffer<T, mwFileMemAllocator<T, 3> > buffer;
};

class mwFileDevice {
public:
    struct Callback {
        void (*function)(unsigned long, unsigned long, unsigned long,
                         unsigned long, void*);
        void* callback_data;
        unsigned long argument0;
        unsigned long argument1;
        unsigned long argument2;
        unsigned long argument3;
    };

    static void serviceCallbacks();
    static void queueErrorCallback(const Callback& callback);
    static void initializeCallbacks(unsigned long size);

private:
    static mwProducerConsumerQueue<Callback> sQueue;
};

class mwFileMountTable {
public:
    static mwFileMountTable& get();
    static int initialize();

    int getMountPointFromName(mwFileMountPoint*& mount_point,
                              const char* name);
    int getMountPointFromName(mwFileMountPoint*& mount_point,
                              const char* first, const char* last) const;

private:
    std::vector<mwFileMountPoint*, mwFileMemAllocator<mwFileMountPoint*, 3> > mounts;
    mutable mwFileMutex mutex;
    static mwFileMountTable* spTable;
};

inline mwFileMountPoint::~mwFileMountPoint()
{
}

mwFileMountPoint::mwFileMountPoint(const char* mount_name)
    : mount_count(0)
{
    strcpy(name, mount_name);
}

namespace {


mwFileServer* mwFileDummyMountPoint::getServer()
{
    return mwFileGetGenericCommandServer();
}
}

int _mwFileMountInit()
{
    int error = mwFileMountTable::initialize();
    int result = 0;
    if (error != 0) {
        result = error;
    }
    return result;
}

void mwFileDevice::serviceCallbacks()
{
    Callback callback;

    while (sQueue.consumeNonBlocking(callback)) {
        callback.function(callback.argument0, callback.argument1,
                          callback.argument2, callback.argument3,
                          callback.callback_data);
    }
}

void mwFileDevice::queueErrorCallback(const Callback& callback)
{
    sQueue.produce(callback);
}


void mwFileDevice::initializeCallbacks(unsigned long size)
{
    sQueue.resize(size);
}

int mwFileMountTable::getMountPointFromName(mwFileMountPoint*& mount_point,
                                            const char* name)
{
    return getMountPointFromName(mount_point, name, name + strlen(name));
}

/* TODO: [near miss] 99.96%; constructor and call-temporary ownership recovered;
 * default comparator source/argument-copy stack slots remain reversed. */
int mwFileMountTable::getMountPointFromName(
    mwFileMountPoint*& mount_point, const char* first, const char* last) const
{
    mwFileMutexLock lock(mutex);
    mwFileDummyMountPoint query(first, last);
    pointer_less<mwFileMountPoint> compare;
    mwFileMountPoint* const* found = std::lower_bound(
        mounts.begin(), mounts.end(), &query, compare);

    if (found == mounts.end()) {
        mount_point = 0;
        return -15;
    }

    mount_point = *found;
    if (mwFileStringCompareIgnoreCase(mount_point->name, query.name) == 0) {
        return 0;
    }
    return -15;
}

namespace {
mwFileDummyMountPoint::~mwFileDummyMountPoint()
{
}
}

mwFileMountTable& mwFileMountTable::get()
{
    return *spTable;
}
