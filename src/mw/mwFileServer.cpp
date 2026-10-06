struct mwFileTypeInfo {
};

class mwFileCommand;
class mwFileServer;

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

#include "mw/mwFileMutex.h"

template <class T>
class mwProducerConsumerQueue {
public:
    mwProducerConsumerQueue();
    ~mwProducerConsumerQueue();
    void produce(T value);
    unsigned char consumeNonBlocking(T& value);
    void resize(unsigned long size);

private:
    unsigned char storage[0x38];
};

template <class T, int Alignment>
class mwFileMemAllocator {
};

namespace std {
template <class T, class Allocator>
class vector {
public:
    vector();
    ~vector();
    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;
    void push_back(const T& value);
    T* erase(T* position);
    void reserve(unsigned long size);

private:
    T* storage;
    unsigned long size_value;
    unsigned long capacity;
};
}

class mwFileServerManagerMultiThreaded {
public:
    void queueCommandCallback(mwFileCommand* command);
    void notifyServerActivityChange(bool active);
};

class mwFileCommand {
public:
    virtual ~mwFileCommand();
    virtual unsigned char isCompleted(void*) const = 0;
    virtual unsigned char isAborted() const = 0;
    virtual void waitForCompletion() const = 0;
    virtual int abort() = 0;
    virtual int service() = 0;

    void serviceCallbackAndPendingDelete();

private:
    unsigned char field_0x04[0x0C];
    int reference_count;
    mwFileServer* server;

    friend class mwFileServerNotQueued;
};

class mwFileServer {
public:
    mwFileServer() : manager(0) {}
    virtual ~mwFileServer();
    virtual bool isInitialized() const = 0;
    virtual int service() = 0;
    virtual int addCommand(mwFileCommand*, unsigned char) = 0;
    virtual bool isActive() const = 0;
    virtual bool queryActive() const = 0;

    void notify(bool active);
    void notifyCommandActive(mwFileCommand* command);
    void notifyCommandAdded();
    void queueCommandCallback(mwFileCommand* command);

protected:
    mwFileServerManagerMultiThreaded* manager;
    mwFileMutex mutex;
};

class mwFileServerNotQueued : public mwFileServer {
public:
    mwFileServerNotQueued();
    virtual ~mwFileServerNotQueued();
    virtual bool isInitialized() const;
    virtual int service();
    virtual int addCommand(mwFileCommand*, unsigned char);
    virtual bool isActive() const;
    virtual bool queryActive() const;

    int initialize(unsigned short command_count);

private:
    mwProducerConsumerQueue<mwFileCommand*> pending_commands;
    std::vector<mwFileCommand*, mwFileMemAllocator<mwFileCommand*, 3> > active_commands;
    bool initialized;
    unsigned char servicing;
};

class mwFileServerQueued : public mwFileServer {
public:
    virtual ~mwFileServerQueued();
    virtual bool isInitialized() const;
    virtual int service();
    virtual int addCommand(mwFileCommand*, unsigned char);
    virtual bool isActive() const;
    virtual bool queryActive() const;

private:
    unsigned char field_0x20[0x0C];
    void* queue0;
    void* queue1;
};

bool mwFileServerQueued::isActive() const
{
    return queryActive();
}

bool mwFileServerQueued::isInitialized() const
{
    return queue0 != 0 && queue1 != 0;
}

bool mwFileServerNotQueued::isActive() const
{
    return queryActive();
}

int mwFileServerNotQueued::addCommand(mwFileCommand* command,
                                      unsigned char)
{
    {
        mwFileMutexLock lock(mutex);
        command->server = this;
        pending_commands.produce(command);
    }
    notifyCommandAdded();
    return 0;
}

bool mwFileServerNotQueued::isInitialized() const
{
    return initialized;
}

int mwFileServerNotQueued::initialize(unsigned short command_count)
{
    pending_commands.resize(command_count);
    active_commands.reserve(command_count);
    initialized = true;
    return 0;
}

mwFileServerNotQueued::mwFileServerNotQueued()
    : initialized(false), servicing(false)
{
}

mwFileServer::~mwFileServer()
{
}

void mwFileServer::notify(bool active)
{
    if (active) {
        manager->notifyServerActivityChange(active);
    }
}

void mwFileServer::notifyCommandActive(mwFileCommand*)
{
    notify(queryActive());
}



void mwFileServer::notifyCommandAdded()
{
    notify(queryActive());
}

void mwFileServer::queueCommandCallback(mwFileCommand* command)
{
    manager->queueCommandCallback(command);
}

void _mwFileNoOp(...)
{
}

unsigned char mwFileQueryable::isA(mwFileTypeInfo*,
                                    const void*& object) const
{
    object = 0;
    return 0;
}

unsigned char mwFileQueryable::isA(mwFileTypeInfo* type, void*& object)
{
    const void* const_object;
    if (isA(type, const_object)) {
        object = const_cast<void*>(const_object);
        return true;
    }
    return false;
}
