#include "runtime/cstring.h"
#include "mw/mwFile.h"
#include "mw/mwFileContainers.h"
#include "mw/mwFileMutex.h"

extern int mwFileStringCompareIgnoreCase(const char*, const char*);

class mwFileServer;

struct mwFileTypeInfo {
};

template <class T>
class mwFileClassTypeInfo {
public:
    static mwFileTypeInfo sTypeInfo;
};

extern void _mwFileNoOp(...);

extern "C" int mwFileUnmount(const char* mount);

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
    friend class mwFileMountTable;
    friend struct pointer_less<mwFileMountPoint>;
    char name[36];
    volatile int mount_count;

public:
    volatile int& referenceCount()
    {
        return mount_count;
    }
};

extern mwFileServer* mwFileGetGenericCommandServer();

class mwFileMountTable {
public:
    static mwFileMountTable& get();
    static int initialize();

    static void* operator new(unsigned long size, mwTargetMemAlign alignment,
                              const char* name)
    {
        return mwFileMultithreadedMemTraits::allocate(size, alignment, name);
    }

    int getMountPointFromName(mwFileMountPoint*& mount_point,
                              const char* name);
    int getMountPointFromName(mwFileMountPoint*& mount_point,
                              const char* first, const char* last) const;
    int getMountPointFromPath(mwFileMountPoint*& mount_point,
                              const char*& remainder, const char* path) const;
    int addMountPoint(mwFileMountPoint* mount_point);
    int removeMountPoint(mwFileMountPoint* mount_point);
    mwFileMountTable();
    ~mwFileMountTable();

private:
    std::vector<mwFileMountPoint*, mwFileMemAllocator<mwFileMountPoint*, 3> > mounts;
    mutable mwFileMutex mutex;

public:
    static mwFileMountTable* spTable;
};

extern int _mwFilePlatformInterlockedIncrement(volatile int& value);
extern int _mwFilePlatformInterlockedDecrement(volatile int& value);

namespace {
class mwFileDirectoryMountPoint : public mwFileMountPoint {
public:
    mwFileDirectoryMountPoint(const char* mount_name);
    virtual ~mwFileDirectoryMountPoint();
    virtual int startOpenFileCommand(mwFileCommand*&, const char*,
                                     unsigned long, mwFileCallback, void*);
    virtual int convertToPlatformPath(char*, const char*, unsigned long,
                                      unsigned long&);
    virtual const char* getInternalPath() const;
    virtual mwFileServer* getServer();
    int initialize(const char* path);

private:
    mwFileMountPoint* parent;
    std::basic_string<char, std::char_traits<char>, mwFileMemAllocator<char, 3> > directory;
};

mwFileServer* mwFileDirectoryMountPoint::getServer()
{
    return parent->getServer();
}

/* TODO: [near miss] 97.35%; Allocator() default-argument temp is passed via mr r4,r3 where retail re-materializes the stack address. */
int mwFileDirectoryMountPoint::convertToPlatformPath(
    char* buffer, const char* path, unsigned long size,
    unsigned long& required)
{
    std::basic_string<char, std::char_traits<char>, mwFileMemAllocator<char, 3> > full_path;
    full_path.reserve(directory.length() + strlen(path));
    full_path.assign(directory, 0, -1);
    full_path.append(path, strlen(path));

    mwFileMountPoint* mount_point;
    int error = mwFileMountTable::spTable->getMountPointFromPath(
        mount_point, path, full_path.data());
    if (error != 0) {
        return error;
    }
    return mount_point->convertToPlatformPath(buffer, path, size, required);
}

/* TODO: [near miss] 97.43%; same Allocator() default-argument temp staging as convertToPlatformPath. */
int mwFileDirectoryMountPoint::startOpenFileCommand(
    mwFileCommand*& command, const char* path, unsigned long flags,
    mwFileCallback callback, void* callback_data)
{
    std::basic_string<char, std::char_traits<char>, mwFileMemAllocator<char, 3> > full_path;
    full_path.reserve(directory.length() + strlen(path));
    full_path.assign(directory, 0, -1);
    full_path.append(path, strlen(path));

    mwFileMountPoint* mount_point;
    int error = mwFileMountTable::spTable->getMountPointFromPath(
        mount_point, path, full_path.data());
    if (error != 0) {
        return error;
    }
    return mount_point->startOpenFileCommand(command, path, flags, callback,
                                             callback_data);
}

const char* mwFileDirectoryMountPoint::getInternalPath() const
{
    return directory.data();
}

/* TODO: [breakthrough needed] 91.69%; level=1 keeps append(const char*) out of line (traits::length call);
 * retail inlines it as strlen + append(s, n). */
int mwFileDirectoryMountPoint::initialize(const char* path)
{
    const char* remainder;
    int error = mwFileMountTable::spTable->getMountPointFromPath(
        parent, remainder, path);
    if (error != 0) {
        return error;
    }

    _mwFilePlatformInterlockedIncrement(parent->referenceCount());
    directory = parent->getInternalPath();
    directory.append(remainder, strlen(remainder));
    std::replace(directory.begin(), directory.end(), '\\', '/');
    if (directory[directory.length() - 1] != '/') {
        directory.append("/");
    }
    return 0;
}

/* TODO: [breakthrough needed] 89.73%; ~mwFileMountPoint (T, defined later) is called, retail inlines it; level=1 base inlining unresolved. */
mwFileDirectoryMountPoint::~mwFileDirectoryMountPoint()
{
    if (parent != 0) {
        _mwFilePlatformInterlockedDecrement(parent->referenceCount());
        parent = 0;
    }
}

/* TODO: [breakthrough needed] 70.81%; base mwFileMountPoint ctor not inlined (level=1: base of polymorphic mwFileQueryable never inlines); retail inlines it. */
mwFileDirectoryMountPoint::mwFileDirectoryMountPoint(const char* mount_name)
    : mwFileMountPoint(mount_name), parent(0)
{
}
}

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

/* TODO: [breakthrough needed] 34.59%; level=1 never inlines a base ctor whose own base is polymorphic (mwFileQueryable); retail inlines mwFileMountPoint(first, last). */
mwFileDummyMountPoint::mwFileDummyMountPoint(const char* first,
                                             const char* last)
    : mwFileMountPoint(first, last)
{
}
}

class mwFileDevice {
public:
    virtual ~mwFileDevice();
    virtual void setErrorCallback(mwFileErrorCallback, void*) = 0;

    struct Callback {
        Callback() {}

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


namespace {


}

mwProducerConsumerQueue<mwFileDevice::Callback> mwFileDevice::sQueue;

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

mwFileDevice::~mwFileDevice()
{
}

int mwFileSetErrorCallback(const char* mount, mwFileErrorCallback callback,
                           void* callback_data)
{
    mwFileMountTable* table = mwFileMountTable::spTable;
    mwFileMountPoint* mount_point;
    int error = table->getMountPointFromName(mount_point, mount,
                                             mount + strlen(mount));
    if (error != 0) {
        return error;
    }

    void* queried_object;
    mwFileDevice* device;
    if (mount_point->isA(&mwFileClassTypeInfo<mwFileDevice>::sTypeInfo,
                         queried_object)) {
        device = static_cast<mwFileDevice*>(queried_object);
    } else {
        device = 0;
    }

    if (device != 0) {
        device->setErrorCallback(callback, callback_data);
        return 0;
    }
    return -28;
}

int mwFileUnmount(const char* mount)
{
    mwFileMountTable* table = mwFileMountTable::spTable;
    mwFileMountPoint* mount_point;
    int error = table->getMountPointFromName(mount_point, mount,
                                             mount + strlen(mount));
    if (error != 0) {
        _mwFileNoOp("mwFile - ERROR - could not find mount point %s\n", mount);
        return error;
    }

    if (mount_point->referenceCount() > 0) {
        _mwFileNoOp("mwFile: ERROR - can not unmount a busy mount point (something else is mounted to it)\n");
        return -25;
    }

    error = mwFileMountTable::spTable->removeMountPoint(mount_point);
    if (error != 0) {
        _mwFileNoOp("mwFile: ERROR - unable to remove mount point %s - %d\n", mount, error);
        return error;
    }
    return 0;
}

int mwFileMountPath(const char* mount, const char* path)
{
    if (strlen(mount) > 32) {
        return -15;
    }

    mwFileDirectoryMountPoint* mount_point = new (MW_TARGET_MEM_ALIGN_32, "Directory mount point") mwFileDirectoryMountPoint(mount);
    if (mount_point == 0) {
        return -10;
    }

    int error = mount_point->initialize(path);
    if (error != 0) {
        delete mount_point;
        return error;
    }

    error = mwFileMountTable::spTable->addMountPoint(mount_point);
    if (error != 0) {
        delete mount_point;
        return error;
    }
    return 0;
}

int mwFileMountTable::getMountPointFromPath(mwFileMountPoint*& mount_point,
                                            const char*& remainder,
                                            const char* path) const
{
    mwFileMutexLock lock(mutex);
    if (*path != '/') {
        return -15;
    }

    const char* end = strchr(path + 1, '/');
    if (end == 0) {
        end = path + strlen(path + 1);
        end++;
    }
    unsigned long length = end - (path + 1);
    if (length > 32) {
        return -15;
    }

    int error = getMountPointFromName(mount_point, path + 1, end);
    if (error != 0) {
        remainder = 0;
        return error;
    }

    if (*end != '\0') {
        remainder = end + 1;
    } else {
        remainder = end;
    }
    return 0;
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

/* TODO: [near miss] 99.94%; pointer_less local and its by-value copy trade stack slots 0xc/0x10. */
int mwFileMountTable::removeMountPoint(mwFileMountPoint* mount_point)
{
    pointer_less<mwFileMountPoint> compare;
    mwFileMutexLock lock(mutex);
    mwFileMountPoint** found = std::lower_bound(
        mounts.begin(), mounts.end(), mount_point, compare);

    if (found == mounts.end()) {
        return -15;
    }

    delete *found;
    mounts.erase(found);
    return 0;
}

/* TODO: [near miss] 99.94%; pointer_less local and its by-value copy trade stack slots 0xc/0x10. */
int mwFileMountTable::addMountPoint(mwFileMountPoint* mount_point)
{
    pointer_less<mwFileMountPoint> compare;
    mwFileMutexLock lock(mutex);
    mwFileMountPoint** found = std::lower_bound(
        mounts.begin(), mounts.end(), mount_point, compare);

    if (found != mounts.end()) {
        if (mwFileStringCompareIgnoreCase((*found)->name,
                                          mount_point->name) == 0) {
            return -14;
        }
    }

    mounts.insert(found, mount_point);
    return 0;
}

mwFileMountTable::~mwFileMountTable()
{
    if (!mounts.empty()) {
        _mwFileNoOp("mwFile: ERROR - shutting down with locked mount points\n");
    }
    for (mwFileMountPoint** it = mounts.begin(); it != mounts.end(); ++it) {
        _mwFileNoOp("mwFile: ERROR - forcing unmount of %s\n", (*it)->name);
        delete *it;
    }
}

mwFileMountTable::mwFileMountTable()
{
    mounts.reserve(10);
}

inline mwFileMountPoint::~mwFileMountPoint()
{
}

mwFileMountPoint::mwFileMountPoint(const char* mount_name)
    : mount_count(0)
{
    strcpy(name, mount_name);
}

int mwFileMountTable::initialize()
{
    spTable = new (MW_TARGET_MEM_ALIGN_32, "mwFile mount point table") mwFileMountTable;
    if (spTable == 0) {
        return -10;
    }
    return 0;
}

mwFileMountTable* mwFileMountTable::spTable;

mwFileMountTable& mwFileMountTable::get()
{
    return *spTable;
}

mwFileServer* mwFileDummyMountPoint::getServer()
{
    return mwFileGetGenericCommandServer();
}

namespace {
/* TODO: [breakthrough needed] 64.76%; ~mwFileMountPoint not inlined under level=1; retail inlines base dtor. */
mwFileDummyMountPoint::~mwFileDummyMountPoint()
{
}
}
