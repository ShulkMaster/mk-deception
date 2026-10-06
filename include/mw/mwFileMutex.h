#ifndef MW_MWFILEMUTEX_H
#define MW_MWFILEMUTEX_H

#include "dolphin/os.h"

class mwFileMutex : public OSMutex {
public:
    mwFileMutex();
    ~mwFileMutex();
    void lock();
    void unlock();
};

class mwFileMutexLock {
public:
    explicit mwFileMutexLock(mwFileMutex& value) : mutex(&value)
    {
        mutex->lock();
    }

    ~mwFileMutexLock()
    {
        mutex->unlock();
    }

private:
    mwFileMutex* mutex;
    mwFileMutexLock(const mwFileMutexLock&);
    mwFileMutexLock& operator=(const mwFileMutexLock&);
};

#endif
