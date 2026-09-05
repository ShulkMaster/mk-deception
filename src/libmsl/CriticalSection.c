#include "msl/CriticalSection.h"
#include "dolphin/mutex.h"
#include "msl/mslsupport.h"

MslCriticalSection* g_CriticalSectionDebug_List;
OSMutex s_CriticalSectionDebug_SystemMutex;
int s_CriticalSectionDebug_SystemMutexInitialized;

static int AddRequestingCS_ByThread(
    MslCriticalSection* requested, void* thread);
static void DebugDump_CriticalCodeSection(
    MslCriticalSection* context, const char* file, int line);

/* Exact: retail owner assertion, recursion post-decrement, and unlock. */
int LeaveCriticalCodeSection_DEBUG(
    MslCriticalSection* section, const char* file, int line) {
    int old_count;

    if (OSGetCurrentThread() != section->owner_thread) {
        mslDebugPrintf(
            "ASSERTION FAILED (%s): %s:%d\n",
            "CS_GetCurrentThreadId() == cs->threadId",
            "CriticalSection.c", 0x276);
        while (1) {
        }
    }
    if (section->reentry_count <= 0) {
        mslDebugPrintf(
            "ASSERTION FAILED (%s): %s:%d\n",
            "cs->reentry_count > 0", "CriticalSection.c",
            0x277);
        while (1) {
        }
    }

    old_count = section->reentry_count--;
    if (section->reentry_count == 0) {
        section->owner_thread = 0;
        OSUnlockMutex(&section->mutex);
    }
    return old_count;
}

/*
 * Exact: retail mutex acquisition, waiter publication, dependency checks,
 * diagnostics, waiter cleanup, and recursion state.
 */
int EnterCriticalCodeSection_DEBUG(
    MslCriticalSection* section, const char* file, int line) {
    void* thread;

    if (section == 0) {
        mslDebugPrintf(
            "ASSERTION FAILED (%s): %s:%d\n",
            "NULL != cs", "CriticalSection.c", 0x1DB);
        while (1) {
        }
    }

    thread = OSGetCurrentThread();
    if (thread == section->owner_thread) {
        if (section->reentry_count < 10) {
            section->lock_files[section->reentry_count] = file;
            section->lock_lines[section->reentry_count] = line;
        }
        section->reentry_count++;
    } else {
        void* waiting_on;
        int spins;
        int locked;
        int i;

        if (AddRequestingCS_ByThread(section, thread) != 0) {
            DebugDump_CriticalCodeSection(section, file, line);
        }

        waiting_on = section->owner_thread;
        locked = OSTryLockMutex(&section->mutex);
        spins = 0;
        while (waiting_on == 0 && locked == 0) {
            waiting_on = section->owner_thread;
            locked = OSTryLockMutex(&section->mutex);
            OSYieldThread();
            spins++;
            if (spins > 1000) {
                mslDebugPrintf(
                    "Stuck in semaphore %08x - THREAD: %08x "
                    "Waits on %08x\n",
                    section, thread, section->owner_thread);
                spins = 0;
                DebugDump_CriticalCodeSection(section, file, line);
            }
        }

        if (locked == 0) {
            OSLockMutex(&s_CriticalSectionDebug_SystemMutex);
            for (i = 0; i < 10; i++) {
                if (section->waiting_threads[i] == 0) {
                    section->waiting_threads[i] = thread;
                    i = 11;
                }
            }
            OSUnlockMutex(&s_CriticalSectionDebug_SystemMutex);

            mslDebugPrintf(
                "Wait on semaphore %08x - THREAD: %08x "
                "Waits on %08x\n",
                section, thread, waiting_on);

            spins = 0;
            locked = OSTryLockMutex(&section->mutex);
            while (locked == 0) {
                OSYieldThread();
                spins++;
                if (spins == 2000) {
                    mslDebugPrintf(
                        "Stuck in semaphore %08x - THREAD: %08x "
                        "Waits on %08x\n",
                        section, thread, waiting_on);
                    DebugDump_CriticalCodeSection(
                        section, file, line);
                }
                locked = OSTryLockMutex(&section->mutex);
            }

            mslDebugPrintf(
                "Acquired semaphore %08x - THREAD: %08x \n",
                section, thread);
            OSLockMutex(&s_CriticalSectionDebug_SystemMutex);
            for (i = 0; i < 10; i++) {
                void** waiting_slot =
                    &section->waiting_threads[i];

                if (thread == *waiting_slot) {
                    *waiting_slot = 0;
                    i = 11;
                }
            }
            OSUnlockMutex(&s_CriticalSectionDebug_SystemMutex);
        }

        if (section->owner_thread != 0) {
            mslDebugPrintf(
                "ERROR: EnterCriticalSection THID NOT CLEARED\n");
        }
        if (section->reentry_count != 0) {
            mslDebugPrintf(
                "ERROR: EnterCriticalSection COUNT NOT CLEARED\n");
        }

        section->owner_thread = thread;
        section->reentry_count = 1;
        section->lock_files[0] = file;
        section->lock_lines[0] = line;
    }

    return section->reentry_count;
}

void UnInitCriticalSection(
    MslCriticalSection* section) {
    MslCriticalSection* current;
    MslCriticalSection** link;

    OSLockMutex(&s_CriticalSectionDebug_SystemMutex);
    current = g_CriticalSectionDebug_List;
    link = &g_CriticalSectionDebug_List;
    while (current != 0) {
        if (current == section) {
            *link = current->next;
            current = 0;
        } else {
            link = &current->next;
            current = current->next;
        }
    }
    OSUnlockMutex(&s_CriticalSectionDebug_SystemMutex);
}

/* TODO: [near miss] 99.61%; all stores and list publication agree;
 * only the epilogue's saved-register/LR load order differs (248 bytes). */
int InitCriticalCodeSection_DEBUG(
    MslCriticalSection* section, const char* file, int line) {
    int i;

    OSInitMutex(&section->mutex);
    section->reentry_count = 0;
    section->owner_thread = 0;
    section->creation_line = line;
    section->creation_file = file;

    if (s_CriticalSectionDebug_SystemMutexInitialized == 0) {
        s_CriticalSectionDebug_SystemMutexInitialized = 1;
        OSInitMutex(&s_CriticalSectionDebug_SystemMutex);
    }

    OSLockMutex(&s_CriticalSectionDebug_SystemMutex);
    section->next = g_CriticalSectionDebug_List;
    g_CriticalSectionDebug_List = section;
    OSUnlockMutex(&s_CriticalSectionDebug_SystemMutex);

    for (i = 0; i < 10; i++) {
        section->waiting_threads[i] = 0;
    }
    for (i = 0; i < 10; i++) {
        section->dependencies[i] = 0;
    }
    return 1;
}

/* TODO: [near miss] 92.64%; dependency insertion and interlock CFG agree;
 * GPR homes and one redundant requested-pointer copy differ (300/296 bytes). */
static int AddRequestingCS_ByThread(
    MslCriticalSection* requested, void* thread) {
    MslCriticalSection* owned;
    int inserted = 0;
    int interlock = 0;
    int i;

    OSLockMutex(&s_CriticalSectionDebug_SystemMutex);
    for (owned = g_CriticalSectionDebug_List;
         owned != 0; owned = owned->next) {
        if (owned != requested && owned->owner_thread == thread) {
            for (i = 0; i < 10; i++) {
                if (owned->dependencies[i] == 0) {
                    owned->dependencies[i] = requested;
                    inserted = i + 1;
                }
                if (owned->dependencies[i] == requested) {
                    i = 10;
                }
            }
        }
    }

    if (inserted != 0) {
        for (i = 0; i < 10; i++) {
            MslCriticalSection* dependency =
                requested->dependencies[i];
            int j;

            if (dependency != 0) {
                for (j = 0; j < 10; j++) {
                    if (dependency->dependencies[j] == requested) {
                        mslDebugPrintf(
                            "MSL CRITICAL SECTION INTERLOCK POSSIBLE: "
                            "0x%08x <--> 0x%08x\n",
                            requested, dependency);
                        interlock = 1;
                    }
                }
            }
        }
    }

    OSUnlockMutex(&s_CriticalSectionDebug_SystemMutex);
    return interlock;
}

static void DebugDump_CriticalCodeSection(
    MslCriticalSection* context, const char* file, int line) {
    int i;
    int count;
    MslCriticalSection* section;

    mslDebugPrintf(
        "MSL DEBUG CRIT SEC: CONTEXT 0x%08x- %s:%d\n",
        context, file, line);
    OSLockMutex(&s_CriticalSectionDebug_SystemMutex);

    for (section = g_CriticalSectionDebug_List;
         section != 0; section = section->next) {
        mslDebugPrintf(
            "  - CS: 0x%08x created %s:%d\n"
            "    Owner: 0x%08x Locks: %d\n",
            section, section->creation_file, section->creation_line,
            section->owner_thread, section->reentry_count);

        count = section->reentry_count;
        if (count > 10) {
            count = 10;
        }
        for (i = 0; i < count; i++) {
            mslDebugPrintf(
                "       Locks:  %s : %d\n",
                section->lock_files[i], section->lock_lines[i]);
        }

        mslDebugPrintf("    WAITING:\n");
        for (i = 0; i < 10; i++) {
            if (section->waiting_threads[i] != 0) {
                mslDebugPrintf(
                    "       Waits:  0x%08x\n",
                    section->waiting_threads[i]);
            }
        }

        mslDebugPrintf("    DEPENDENCIES:\n");
        for (i = 0; i < 10; i++) {
            if (section->dependencies[i] != 0) {
                mslDebugPrintf(
                    "       CS:  0x%08x\n",
                    section->dependencies[i]);
            }
        }
    }

    OSUnlockMutex(&s_CriticalSectionDebug_SystemMutex);
}
