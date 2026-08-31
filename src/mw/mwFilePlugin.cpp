struct mwFileTypeInfo;
struct mwFile;
class mwFileCommand;
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

class mwFileQueryable {
public:
    virtual unsigned char isA(mwFileTypeInfo*, void*&);
    virtual unsigned char isA(mwFileTypeInfo*, const void*&) const;
};

class mwFileMultithreadedMemTraits {
public:
    static void deallocate(void*);
};

class mwFileCommand : public mwFileQueryable {
public:
    mwFileCommand(mwFileCallback, void*, mwFile*);
    virtual ~mwFileCommand();

    virtual unsigned char isCompleted(mwFileAsyncResult&) const = 0;
    virtual unsigned char isAborted() const = 0;
    virtual mwFileAsyncResult waitForCompletion() const = 0;
    virtual int abort() = 0;
    virtual void service() = 0;
    virtual void serviceCallback() = 0;
    virtual void deleteSelf() = 0;
    virtual int close() = 0;

    void handleCompletion(mwFileAsyncResult);

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
    virtual void service() = 0;
    virtual void serviceCallback();
    virtual void deleteSelf();
    virtual int close();
    virtual mwFileAsyncResult getResult() const = 0;
    virtual ~mwFileCommandPluginBase();

    int handleSignalState();

private:
    unsigned char aborted;
    volatile int signal_state;
};

typedef char mwFileCommandSizeCheck[sizeof(mwFileCommand) == 0x20 ? 1 : -1];
typedef char mwFileCommandPluginBaseSizeCheck[
    sizeof(mwFileCommandPluginBase) == 0x28 ? 1 : -1];

extern void _mwFileTickEx(bool);

void mwFileCommandPluginBase::deleteSelf()
{
    if (signal_state != 1 || callback_depth != 0) {
        pending_delete = 1;
    } else {
        close();
        delete this;
    }
}

void mwFileCommandPluginBase::serviceCallback()
{
    signal_state = 1;
    handleCompletion(getResult());
}

int mwFileCommandPluginBase::handleSignalState()
{
    return 4;
}

int mwFileCommandPluginBase::abort()
{
    if (signal_state != 2 && signal_state != 1) {
        aborted = 1;
    }
    return 0;
}

unsigned char mwFileCommandPluginBase::isAborted() const
{
    return aborted;
}

unsigned char mwFileCommandPluginBase::isCompleted(
    mwFileAsyncResult& result) const
{
    if (signal_state == 1) {
        result = getResult();
        return 1;
    }
    return 0;
}

mwFileAsyncResult mwFileCommandPluginBase::waitForCompletion() const
{
    while (signal_state != 1) {
        _mwFileTickEx(true);
    }
    return getResult();
}

int mwFileCommandPluginBase::close()
{
    return 0;
}

mwFileCommandPluginBase::mwFileCommandPluginBase(
    mwFileCallback completion_callback, void* completion_data,
    mwFile* owning_file)
    : mwFileCommand(completion_callback, completion_data, owning_file),
      aborted(0),
      signal_state(0)
{
}

inline mwFileCommandPluginBase::~mwFileCommandPluginBase()
{
}
