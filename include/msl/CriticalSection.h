#ifndef MSL_CRITICAL_SECTION_H
#define MSL_CRITICAL_SECTION_H

#include "platform/os_types.h"

typedef struct MslCriticalSection {
    int reentry_count;                 /* +0x00 */
    void* owner_thread;                /* +0x04 */
    OSMutex mutex;                     /* +0x08 */
    struct MslCriticalSection* next;   /* +0x20 */
    int creation_line;                 /* +0x24 */
    int lock_lines[10];                /* +0x28 */
    const char* creation_file;         /* +0x50 */
    const char* lock_files[10];        /* +0x54 */
    void* waiting_threads[10];         /* +0x7C */
    struct MslCriticalSection* dependencies[10]; /* +0xA4 */
} MslCriticalSection; /* 0xCC */

typedef char MslCriticalSectionSize[
    sizeof(MslCriticalSection) == 0xCC ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
int InitCriticalCodeSection_DEBUG(
    MslCriticalSection* section, const char* file, int line);
int EnterCriticalCodeSection_DEBUG(
    MslCriticalSection* section, const char* file, int line);
int LeaveCriticalCodeSection_DEBUG(
    MslCriticalSection* section, const char* file, int line);
void UnInitCriticalSection(MslCriticalSection* section);
#ifdef __cplusplus
}
#endif

#endif
