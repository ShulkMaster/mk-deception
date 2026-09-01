#include "runtime/cstring.h"

class mwFileCommand;
class mwFileServer;
struct mwFileTypeInfo;
struct _mwFileAsyncResult;

typedef void (*mwFileCallback)(mwFileCommand*, _mwFileAsyncResult, void*);

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
    virtual ~mwFileMountPoint();
    virtual int startOpenFileCommand(mwFileCommand*&, const char*,
                                     unsigned long, mwFileCallback, void*) = 0;
    virtual int convertToPlatformPath(char*, const char*, unsigned long,
                                      unsigned long&) = 0;
    virtual const char* getInternalPath() const = 0;
    virtual mwFileServer* getServer() = 0;

protected:
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
    : mwFileMountPoint("")
{
    unsigned long length = last - first;
    memcpy(name, first, length);
    name[length] = '\0';
}
}

template <class T>
class mwProducerConsumerQueue {
public:
    unsigned char consumeNonBlocking(T& value);
    void produce(T value);
    void resize(unsigned long size);
};

class mwFileDevice {
public:
    struct Callback {
        typedef void (*Function)(unsigned long, unsigned long, unsigned long,
                                 unsigned long, void*);

        Function function;
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
    static mwFileMountTable* spTable;
};

mwFileMountPoint::~mwFileMountPoint()
{
}

mwFileMountPoint::mwFileMountPoint(const char* mount_name)
    : mount_count(0)
{
    strcpy(name, mount_name);
}

namespace {
mwFileDummyMountPoint::~mwFileDummyMountPoint()
{
}

mwFileServer* mwFileDummyMountPoint::getServer()
{
    return mwFileGetGenericCommandServer();
}
}

int _mwFileMountInit()
{
    int error = mwFileMountTable::initialize();
    return error != 0 ? error : 0;
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

mwFileMountTable& mwFileMountTable::get()
{
    return *spTable;
}
