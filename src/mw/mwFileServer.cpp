/* TODO: [review] layout stub: mwFileServerQueued::test() restores a linker-stripped retail
 * function (MAP UNUSED, 0x2A0) only to own the "TestCommand" string at @stringBase0+0 and the
 * mwFileFailCommand<0, 0> instantiation; its body is NOT recovered source. */
#include "mw/mwFile.h"
#include "mw/mwFileQueue.h"
#include "mslcpp/algorithm.h"

#include "runtime/cstring.h"

struct mwFileTypeInfo {
};

class mwFileServer;
class mwFileServerManagerMultiThreaded;

inline void _mwFileNoOp(...)
{
}

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo* type, void*& object);
    virtual unsigned char isA(mwFileTypeInfo* type, const void*& object) const;
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

    void serviceCallbackAndPendingDelete();
    void handleCompletion(mwFileAsyncResult);

    static void* operator new(unsigned long size, mwTargetMemAlign alignment, const char* name)
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

    friend class mwFileServerQueued;
    friend class mwFileServerNotQueued;
};

template <unsigned long ResultValue, int ResultError>
class mwFileFailCommand : public mwFileCommand {
public:
    mwFileFailCommand(mwFileCallback completion_callback, void* completion_data,
                      mwFile* owning_file)
        : mwFileCommand(completion_callback, completion_data, owning_file), completed(false)
    {
    }

    virtual ~mwFileFailCommand() {}

    virtual unsigned char isCompleted(mwFileAsyncResult& result) const
    {
        result.value.bytes = ResultValue;
        result.error = ResultError;
        return completed;
    }

    virtual unsigned char isAborted() const { return 0; }

    virtual mwFileAsyncResult waitForCompletion() const
    {
        mwFileAsyncResult result;
        while (!isCompleted(result)) {
            mwFileTick();
        }
        return result;
    }

    virtual int abort() { return 0; }
    virtual int service() { return 2; }

    virtual void serviceCallback()
    {
        completed = true;
        handleCompletion(waitForCompletion());
    }

    virtual void deleteSelf()
    {
        if (!completed || callback_depth != 0) {
            pending_delete = 1;
        } else {
            close();
            delete this;
        }
    }

    virtual int close() { return 0; }

private:
    bool completed;
};

class mwFileDevice {
public:
    static void serviceCallbacks();
};

extern bool _mwFilePlatformIsInServiceThread();
extern void _mwFilePlatformYieldTimeSlice();

class mwFileServer {
public:
    mwFileServer() : manager(0) {}
    virtual ~mwFileServer() {}
    virtual bool isInitialized() const = 0;
    virtual int service() = 0;
    virtual int addCommand(mwFileCommand*, unsigned char) = 0;
    virtual bool isActive() const = 0;
    virtual bool queryActive() const = 0;

    void setManager(mwFileServerManagerMultiThreaded* value);
    void notify(bool active);
    void notifyCommandActive(mwFileCommand* command);
    void notifyCommandAdded();
    void queueCommandCallback(mwFileCommand* command);

protected:
    mwFileServerManagerMultiThreaded* manager;
    mutable mwFileMutex mutex;

    friend class mwFileServerManagerBase;
};

class mwFileServerQueued : public mwFileServer {
public:
    struct QueueItem {
        bool operator>(const QueueItem& x) const
        {
            if ((x.priority & 3u) < (priority & 3u)) {
                return true;
            }
            if ((x.priority & 3u) == (priority & 3u)) {
                return x.sequence < sequence;
            }
            return false;
        }

        mwFileCommand* command;
        unsigned char priority;
        long sequence;
    };

    struct Queue {
        Queue() : sequence(0), queue(mwFileMemAllocator<QueueItem, 3>()) {}

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

        void reserve(unsigned long n) { queue.reserve(n); }
        bool empty() const { return queue.empty(); }
        const QueueItem& top() const { return queue.top(); }
        void pop() { queue.pop(); }

        void push(QueueItem item)
        {
            item.sequence = sequence++;
            queue.push(item);
        }

        long sequence;
        reserve_priority_queue<QueueItem, std::greater<QueueItem> > queue;
    };

    mwFileServerQueued();
    virtual ~mwFileServerQueued();
    virtual bool isInitialized() const;
    virtual int service();
    virtual int addCommand(mwFileCommand*, unsigned char);
    virtual bool isActive() const;
    virtual bool queryActive() const;

    int initialize(unsigned short command_count);
    void shutdown();
    void test();

private:
    mwFileCommand* active[3];
    Queue* pending;
    Queue* ready;
    int active_count;
    bool servicing;
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
    bool servicing;
};

class mwFileServerManagerBase {
public:
    mwFileServerManagerBase();

    void addServer(mwFileServer* server);
    void removeServer(mwFileServer* server);

protected:
    std::vector<mwFileServer*, mwFileMemAllocator<mwFileServer*, 3> > servers;
    mwFileMutex mutex;
};

class mwFileServerManagerMultiThreaded : public mwFileServerManagerBase {
public:
    mwFileServerManagerMultiThreaded();

    void initialize(unsigned long callback_count);
    void service();
    void serviceThread();
    bool isActive() const;
    void serviceCommands();
    void notifyServerActivityChange(bool active);
    void queueCommandCallback(mwFileCommand* command);

private:
    mwFileCondition condition;
    mwFileEvent finished;
    bool quit;
    bool running;
    mwProducerConsumerQueue<mwFileCommand*> callbacks;
};

bool mwFileServerQueued::queryActive() const
{
    mwFileMutexLock lock(mutex);
    if (!pending->empty()) {
        return true;
    }
    for (unsigned char i = 0; i <= 2; i++) {
        if (active[i] != 0) {
            return active[i]->reference_count > 0;
        }
    }
    return false;
}

bool mwFileServerQueued::isActive() const
{
    return queryActive();
}

int mwFileServerQueued::service()
{
    bool more = true;

    if (servicing) {
        return 0;
    }
    servicing = true;
    while (more) {
        {
            mwFileMutexLock lock(mutex);
            while (!pending->empty()) {
                ready->push(pending->top());
                pending->pop();
            }
        }
        if (ready->empty()) {
            if (active_count == 0) {
                servicing = false;
                return 0;
            }
        } else if (active_count < 3) {
            unsigned char priority = ready->top().priority;
            mwFileMutexLock lock(mutex);
            if (active[priority & 3] == 0) {
                active[priority & 3] = ready->top().command;
                active_count++;
                ready->pop();
            }
        }

        mwFileCommand* command = 0;
        unsigned char i;
        for (i = 0; i <= 2; i++) {
            if (active[i] != 0) {
                if (active[i]->reference_count > 0) {
                    command = active[i];
                }
                break;
            }
        }
        if (command == 0) {
            servicing = false;
            return 0;
        }
        switch (command->service()) {
        case 0:
            break;
        case 2: {
            {
                mwFileMutexLock lock(mutex);
                active[i] = 0;
            }
            queueCommandCallback(command);
            active_count--;
            break;
        }
        case 1:
            more = false;
            break;
        }
    }
    servicing = false;
    return 0;
}

void mwFileServerQueued::test()
{
    mwFileCommand* commands[10];
    int i;

    initialize(10);
    for (i = 0; i < 10; i++) {
        commands[i] = new (MW_TARGET_MEM_ALIGN_32, "TestCommand") mwFileFailCommand<0, 0>(0, 0, 0);
        addCommand(commands[i], i);
    }
    while (queryActive()) {
        service();
    }
    for (i = 0; i < 10; i++) {
        commands[i]->deleteSelf();
    }
    shutdown();
}

/* TODO: [near miss] 99.79%; QueueItem temp and by-value copy use swapped stack slots (0xc/0x18). */
int mwFileServerQueued::addCommand(mwFileCommand* command, unsigned char priority)
{
    {
        mwFileMutexLock lock(mutex);
        if (pending == 0) {
            return -4;
        }
        QueueItem item;
        item.command = command;
        item.priority = priority;
        item.sequence = 0;
        pending->push(item);
        command->server = this;
    }
    notifyCommandAdded();
    return 0;
}

bool mwFileServerQueued::isInitialized() const
{
    return pending != 0 && ready != 0;
}

void mwFileServerQueued::shutdown()
{
    delete pending;
    pending = 0;
    delete ready;
    ready = 0;
}

#pragma inline_depth(2)
int mwFileServerQueued::initialize(unsigned short command_count)
{
#pragma inline_depth(1)
    if (pending != 0) {
        return 0;
    }
    pending = new (MW_TARGET_MEM_ALIGN_32, "mwFile priority queue") Queue;
    if (pending == 0) {
        _mwFileNoOp("mwFile: ERROR - could not allocate priority queue\n");
        return -10;
    }
    pending->reserve(command_count);
    ready = new (MW_TARGET_MEM_ALIGN_32, "mwFile internal priority queue") Queue;
    if (ready == 0) {
        _mwFileNoOp("mwFile: ERROR - could not allocate internal priority queue\n");
        return -10;
    }
    ready->reserve(command_count);
    return 0;
}

mwFileServerQueued::~mwFileServerQueued()
{
    delete pending;
    pending = 0;
    delete ready;
    ready = 0;
}

mwFileServerQueued::mwFileServerQueued() : pending(0), active_count(0), servicing(false)
{
    memset(active, 0, sizeof(active));
}

/* TODO: [breakthrough needed] 96.52174%; first-result return copy follows
 * its branch; recover the original shared return boundary. */
bool mwFileServerNotQueued::queryActive() const
{
    bool active;
    {
        mwFileMutexLock lock(mutex);
        mwFileCommand* const* it = active_commands.begin();
        mwFileCommand* const* last = active_commands.end();
        for (; it != last; ++it) {
            if ((*it)->reference_count > 0) {
                break;
            }
        }
        active = it != last;
    }
    if (active) {
        return active;
    }
    bool empty;
    {
        mwFileMutexLock lock(pending_commands.mutex);
        empty = pending_commands.buffer.empty();
    }
    return !empty;
}

bool mwFileServerNotQueued::isActive() const
{
    return queryActive();
}

int mwFileServerNotQueued::addCommand(mwFileCommand* command, unsigned char)
{
    {
        mwFileMutexLock lock(mutex);
        command->server = this;
        pending_commands.produce(command);
    }
    notifyCommandAdded();
    return 0;
}

int mwFileServerNotQueued::service()
{
    mwFileMutexLock lock(mutex);
    if (!servicing) {
        servicing = true;
    } else {
        return 0;
    }

    mwFileCommand* command;
    while (pending_commands.consumeNonBlocking(command)) {
        active_commands.push_back(command);
    }

    mwFileCommand** it = active_commands.begin();
    mwFileCommand** last = active_commands.end();
    while (it != last) {
        switch ((*it)->service()) {
        case 2: {
            mwFileCommand* finished = *it;
            it = active_commands.erase(it);
            last = active_commands.end();
            queueCommandCallback(finished);
            break;
        }
        case 0:
        case 1:
            ++it;
            break;
        }
    }
    {
        mwFileMutexLock flush(mutex);
    }
    servicing = false;
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

mwFileServerNotQueued::~mwFileServerNotQueued()
{
}

mwFileServerNotQueued::mwFileServerNotQueued() : initialized(false), servicing(false)
{
}

void mwFileServerManagerMultiThreaded::queueCommandCallback(mwFileCommand* command)
{
    callbacks.produce(command);
}

void mwFileServerManagerMultiThreaded::notifyServerActivityChange(bool active)
{
    mwFileMutexLock lock(mutex);
    if (active) {
        condition.signal();
    } else {
        mwFileServer** it = servers.begin();
        mwFileServer** last = servers.end();
        for (; it != last; ++it) {
            if ((*it)->queryActive()) {
                break;
            }
        }
        if (it != servers.end()) {
            condition.signal();
        }
    }
}

void mwFileServerManagerMultiThreaded::serviceCommands()
{
    static mwFileServer* sHackServerArray[10];
    mwFileServer** last;

    {
        mwFileMutexLock lock(mutex);
        mwFileServer** source_end = servers.end();
        mwFileServer** source_begin = servers.begin();
        last = std::copy(source_begin, source_end, sHackServerArray);
    }
    for (mwFileServer** it = sHackServerArray; it != last; ++it) {
        if ((*it)->queryActive()) {
            (*it)->service();
        }
    }
}

bool mwFileServerManagerMultiThreaded::isActive() const
{
    for (mwFileServer* const* it = servers.begin(); it != servers.end(); ++it) {
        if ((*it)->isActive()) {
            return true;
        }
    }
    return false;
}

void mwFileServerManagerMultiThreaded::serviceThread()
{
    running = true;
    while (!quit) {
        mutex.lock();
        while (!isActive()) {
            condition.wait(mutex);
        }
        mutex.unlock();
        serviceCommands();
        notifyServerActivityChange(false);
        _mwFilePlatformYieldTimeSlice();
    }
    running = false;
    finished.set();
}

void mwFileServerManagerMultiThreaded::service()
{
    if (!_mwFilePlatformIsInServiceThread()) {
        mwFileDevice::serviceCallbacks();
        mwFileCommand* command;
        while (callbacks.consumeNonBlocking(command)) {
            command->serviceCallbackAndPendingDelete();
        }
    }
}

void mwFileServerManagerMultiThreaded::initialize(unsigned long callback_count)
{
    quit = false;
    finished.reset();
    callbacks.resize(callback_count);
}

mwFileServerManagerMultiThreaded::mwFileServerManagerMultiThreaded()
    : quit(false), running(false)
{
}

/* TODO: [near miss] 97.81%; call order recovered; stop at extra endpoint-result temporary. */
void mwFileServerManagerBase::removeServer(mwFileServer* server)
{
    mwFileMutexLock lock(mutex);
    server->manager = 0;
    mwFileServer** erase_end = servers.end();
    mwFileServer** new_end = std::remove(servers.begin(), servers.end(), server);
    servers.erase(new_end, erase_end);
}

void mwFileServerManagerBase::addServer(mwFileServer* server)
{
    mwFileMutexLock lock(mutex);
    if (std::find(servers.begin(), servers.end(), server) == servers.end()) {
        server->manager = static_cast<mwFileServerManagerMultiThreaded*>(this);
        servers.push_back(server);
    }
}

mwFileServerManagerBase::mwFileServerManagerBase()
{
    servers.reserve(10);
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
