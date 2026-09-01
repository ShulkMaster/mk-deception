#include "dolphin/os.h"
#include "ctype.h"

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

class mwFileGCHandle {
public:
    unsigned long getFileAlign();
    unsigned long getReadAlign();
};

class mwFileGCServer {
public:
    void queryErrorState();

protected:
    unsigned char field_0x00[0x40];
};

class gcnDriver : public mwFileGCServer {
public:
    static void* serviceThreadThunk(void*);
    static void* wakeupThreadThunk(void*);
    void queryErrorState();

    OSThread service_thread;
    unsigned char field_0x358[0x324];
    OSMessageQueue wakeup_queue;
};

class mwFileCommand {
public:
    void wakeup();
};

class mwFileGCCloseCommand;
class mwFileGCReadBufferedCommand;
class mwFileGCReadUnbufferedCommand;

template <class Command>
class mwFileGCIOMixIn {
public:
    int nativeAbort();
};

template <>
int mwFileGCIOMixIn<mwFileGCCloseCommand>::nativeAbort()
{
    return 0;
}

template <>
int mwFileGCIOMixIn<mwFileGCReadBufferedCommand>::nativeAbort()
{
    return 0;
}

template <>
int mwFileGCIOMixIn<mwFileGCReadUnbufferedCommand>::nativeAbort()
{
    return 0;
}

extern void _mwFileServiceThread();
extern gcnDriver& gcnGetDriver();

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
    return *left == '\0' ? -1 : 1;
}

unsigned long mwFileGCHandle::getFileAlign()
{
    return 4;
}

unsigned long mwFileGCHandle::getReadAlign()
{
    return 32;
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

    do {
        OSReceiveMessage(&driver->wakeup_queue,
                         reinterpret_cast<OSMessage*>(&command), 1);
        if (command != 0) {
            command->wakeup();
        }
    } while (command != 0);

    return 0;
}

void gcnDriver::queryErrorState()
{
    mwFileGCServer::queryErrorState();
}

void _mwFilePlatformTick()
{
    gcnGetDriver().queryErrorState();
}

unsigned char _mwFilePlatformIsInServiceThread()
{
    gcnDriver& driver = gcnGetDriver();
    return OSGetCurrentThread() == &driver.service_thread;
}
