/* TODO: [review] layout stubs: mwFileCommand::getError() const reproduces a linker-stripped retail
 * function only to recreate its .data layout; the body is NOT recovered source. Replace with a
 * genuine body if a source turns up. */
#include "mw/mwFile.h"

struct mwFileTypeInfo;
class mwFileServer;

class mwFileQueryable {
public:
    mwFileQueryable()
    {
    }

    virtual unsigned char isA(mwFileTypeInfo* type, void*& object)
    {
        const void* const_object;
        if (isA(type, const_object)) {
            object = const_cast<void*>(const_object);
            return true;
        }
        return false;
    }

    virtual unsigned char isA(mwFileTypeInfo*, const void*& object) const
    {
        object = 0;
        return 0;
    }
};

class mwFileMultithreadedMemTraits {
public:
    static void deallocate(void*);
};

class mwFileServer {
public:
    void notifyCommandActive(mwFileCommand*);
};

extern int _mwFilePlatformInterlockedIncrement(volatile int&);

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

    void serviceCallbackAndPendingDelete();
    void wakeup();
    void handleCompletion(mwFileAsyncResult);
    int getError() const;

    static unsigned short sRetries;

    static void operator delete(void* object)
    {
        if (object != 0) {
            mwFileMultithreadedMemTraits::deallocate(object);
        }
    }

private:
    mwFile* file;
    int error;
    unsigned char callback_depth;
    unsigned char pending_delete;
    unsigned char reserved_0E[2];
    volatile int reference_count;
    mwFileServer* server;
    mwFileCallback callback;
    void* callback_data;
};

typedef char mwFileCommandSizeCheck[sizeof(mwFileCommand) == 0x20 ? 1 : -1];

unsigned short mwFileCommand::sRetries = 10;

void mwFileCommand::serviceCallbackAndPendingDelete()
{
    callback_depth++;
    serviceCallback();

    if (pending_delete) {
        if (callback_depth != 0) {
            callback_depth--;
        }
        deleteSelf();
    } else if (callback_depth != 0) {
        callback_depth--;
    }
}

void mwFileCommand::wakeup()
{
    _mwFilePlatformInterlockedIncrement(reference_count);
    server->notifyCommandActive(this);
}

void mwFileCommand::handleCompletion(mwFileAsyncResult result)
{
    error = result.error;
    if (callback != 0) {
        callback(this, result, callback_data);
    }
}

int mwFileCommand::getError() const
{
    mwFileQueryable base;
    return error;
}

mwFileCommand::~mwFileCommand()
{
    file = 0;
    callback_data = 0;
    server = 0;
}

mwFileCommand::mwFileCommand(
    mwFileCallback completion_callback, void* completion_data,
    mwFile* owning_file)
    : file(owning_file),
      error(0),
      callback_depth(0),
      pending_delete(0),
      reference_count(1),
      server(0),
      callback(completion_callback),
      callback_data(completion_data)
{
}
