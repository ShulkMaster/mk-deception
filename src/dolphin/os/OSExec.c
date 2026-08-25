#include "dolphin/ai.h"
#include "dolphin/cache.h"
#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"

extern void* memset(void* destination, int value, unsigned long size);
extern void* memcpy(void* destination, const void* source, unsigned long size);
extern unsigned long strlen(const char* string);
extern char* strcpy(char* destination, const char* source);
extern int strncmp(const char* first, const char* second, unsigned long count);
extern int sprintf(char* destination, const char* format, ...);

#define BOOT_REGION_START (*(volatile unsigned long*)0x812FDFF0)
#define BOOT_REGION_END (*(volatile unsigned long*)0x812FDFEC)
#define BOOT_FLAG (*(volatile unsigned char*)0x800030E2)
#define OS_EXEC_PARAMS (*(OSExecParams**)0x800030F0)
#define OS_APPLOADER_OFFSET (*(volatile unsigned long*)0x800030F4)
#define PI_REGISTERS ((volatile unsigned long*)0xCC003000)

extern int __OSIsGcam;

static int Prepared;

static int PackArgs(void* address, int argc, char** argv)
{
    int argument_count;
    char* boot_info;
    char* cursor;
    char** list;
    unsigned long i;

    boot_info = (char*)address;
    memset(boot_info, 0, 0x2000);

    if (argc == 0) {
        *(unsigned long*)(boot_info + 8) = 0;
    } else {
        argument_count = argc;
        cursor = boot_info + 0x2000;
        while (--argc >= 0) {
            cursor -= strlen(argv[argc]) + 1;
            strcpy(cursor, argv[argc]);
            argv[argc] = (char*)(cursor - boot_info);
        }

        cursor = boot_info + ((cursor - boot_info) & ~3);
        cursor -= (argument_count + 1) * sizeof(char*);
        list = (char**)cursor;
        for (i = 0; i < (unsigned long)argument_count + 1; i++) {
            list[i] = argv[i];
        }

        cursor -= sizeof(unsigned long);
        *(unsigned long*)cursor = argument_count;
        *(unsigned long*)(boot_info + 8) = cursor - boot_info;
    }

    return 1;
}

/* The retail leaf transfers through LR after sync/isync.  C expresses the
 * control transfer, but cannot reproduce those privileged ordering opcodes. */
static void Run(void* entry_point)
{
    ICFlashInvalidate();
    ((void (*)(void))entry_point)();
}

static void ReadDisc(void* address, int length, int offset)
{
    DVDCommandBlock block;

    DVDReadAbsAsyncPrio(&block, address, length, offset, 0, 0);
    while (DVDGetCommandBlockStatus(&block)) {
        if (!DVDCheckDisk()) {
            __OSDoHotReset(0);
        }
    }
}

static void Callback(long result, DVDCommandBlock* block)
{
    Prepared = 1;
}

void __OSGetExecParams(OSExecParams* params)
{
    if ((unsigned long)OS_EXEC_PARAMS >= 0x80000000) {
        memcpy(params, OS_EXEC_PARAMS, sizeof(OSExecParams));
    } else {
        params->valid = 0;
    }
}

static int GetApploaderPosition(void)
{
    static long apploader_position;
    unsigned long* tgc_header;
    long offset;

    if (apploader_position != 0) {
        return apploader_position;
    }

    if (OS_APPLOADER_OFFSET != 0) {
        tgc_header = OSAllocFromArenaLo(0x40, 32);
        ReadDisc(tgc_header, 0x40, OS_APPLOADER_OFFSET);
        offset = tgc_header[14];
        apploader_position = OS_APPLOADER_OFFSET + offset;
    } else {
        apploader_position = 0x2440;
    }

    return apploader_position;
}

typedef struct AppLoaderHeader {
    char date[16];
    unsigned long entry;
    unsigned long size;
    unsigned long reboot_size;
    unsigned long reserved;
} AppLoaderHeader;

typedef void (*AppInitCallback)(void (*report)(const char*, ...));
typedef int (*AppGetNextCallback)(void**, unsigned long*, unsigned long*);
typedef void* (*AppGetEntryCallback)(void);
typedef void (*AppLoaderCallback)(AppInitCallback*, AppGetNextCallback*,
                                  AppGetEntryCallback*);

static inline void SetExecParams(const OSExecParams* params, OSExecParams* address)
{
    memcpy(address, params, sizeof(OSExecParams));
    OS_EXEC_PARAMS = address;
}

static inline void StartDol(const OSExecParams* params, void* entry)
{
    OSExecParams* working = OSAllocFromArenaLo(sizeof(OSExecParams), 1);

    SetExecParams(params, working);
    PI_REGISTERS[9] = 7;
    OSDisableInterrupts();
    Run(entry);
}

static inline int IsStreamEnabled(void)
{
    return DVDGetCurrentDiskID()->streaming != 0;
}

static inline void StopStreaming(void)
{
    DVDCommandBlock block;

    if (!__OSIsGcam && IsStreamEnabled()) {
        AISetStreamVolLeft(0);
        AISetStreamVolRight(0);
        DVDCancelStreamAsync(&block, 0);
        while (DVDGetCommandBlockStatus(&block)) {
            if (!DVDCheckDisk()) {
                __OSDoHotReset(0);
            }
        }
        AISetStreamPlayState(0);
    }
}

static inline AppLoaderHeader* LoadApploader(void)
{
    AppLoaderHeader* header;

    header = OSAllocFromArenaLo(sizeof(AppLoaderHeader), 32);
    ReadDisc(header, sizeof(AppLoaderHeader), GetApploaderPosition());
    ReadDisc((void*)0x81200000, OSRoundUp32B(header->size),
             GetApploaderPosition() + 0x20);
    ICInvalidateRange((void*)0x81200000, OSRoundUp32B(header->size));
    return header;
}

static inline void* LoadDol(const OSExecParams* params,
                            AppLoaderCallback get_interface)
{
    AppInitCallback app_init;
    AppGetNextCallback app_get_next;
    AppGetEntryCallback app_get_entry;
    void* address;
    unsigned long length;
    unsigned long offset;
    OSExecParams* working;

    get_interface(&app_init, &app_get_next, &app_get_entry);
    working = OSAllocFromArenaLo(sizeof(OSExecParams), 1);
    SetExecParams(params, working);
    app_init(OSReport);
    OSSetArenaLo(working);

    while (app_get_next(&address, &length, &offset) != 0) {
        ReadDisc(address, length, offset);
    }
    return app_get_entry();
}

static inline int IsNewApploader(const AppLoaderHeader* header)
{
    return strncmp(header->date, "2004/02/01", 10) > 0;
}

void __OSBootDolSimple(unsigned long dol_offset, unsigned long restart_code,
                       void* region_start, void* region_end,
                       int args_use_default, int argc, char** argv)
{
    OSExecParams* params;
    void* dol_entry;
    AppLoaderHeader* header;

    OSDisableInterrupts();
    params = OSAllocFromArenaLo(sizeof(OSExecParams), 1);
    params->valid = 1;
    params->restart_code = restart_code;
    params->region_start = region_start;
    params->region_end = region_end;
    params->args_use_default = args_use_default;

    if (!args_use_default) {
        params->args_address = OSAllocFromArenaLo(0x2000, 1);
        PackArgs(params->args_address, argc, argv);
    }

    DVDInit();
    DVDSetAutoInvalidation(1);
    DVDResume();
    Prepared = 0;
    __DVDPrepareResetAsync(Callback);
    __OSMaskInterrupts(0xFFFFFFE0);
    __OSUnmaskInterrupts(0x400);
    OSEnableInterrupts();

    while (Prepared != 1) {
        if (!DVDCheckDisk()) {
            __OSDoHotReset(0);
        }
    }

    StopStreaming();
    header = LoadApploader();
    if (IsNewApploader(header)) {
        if (dol_offset == 0xFFFFFFFF) {
            dol_offset = GetApploaderPosition() + 0x20 + header->size;
        }
        params->boot_dol = dol_offset;
        dol_entry = LoadDol(params, (AppLoaderCallback)header->entry);
        StartDol(params, dol_entry);
    } else {
        BOOT_REGION_START = (unsigned long)region_start;
        BOOT_REGION_END = (unsigned long)region_end;
        BOOT_FLAG = 1;
        ReadDisc((void*)0x81300000, OSRoundUp32B(header->reboot_size),
                 GetApploaderPosition() + 0x20 + header->size);
        ICInvalidateRange((void*)0x81300000,
                          OSRoundUp32B(header->reboot_size));
        OSDisableInterrupts();
        ICFlashInvalidate();
        Run((void*)0x81300000);
    }
}

void __OSBootDol(unsigned long dol_offset, unsigned long restart_code, char** argv)
{
    char dol_offset_string[20];
    int argument_count;
    char** arguments;
    int i;
    void* save_start;
    void* save_end;

    OSGetSaveRegion(&save_start, &save_end);
    sprintf(dol_offset_string, "%d", dol_offset);
    argument_count = 0;
    if (argv != 0) {
        while (argv[argument_count] != 0) {
            argument_count++;
        }
    }

    argument_count++;
    arguments = OSAllocFromArenaLo((argument_count + 1) * sizeof(char*), 1);
    arguments[0] = dol_offset_string;
    for (i = 1; i < argument_count; i++) {
        arguments[i] = argv[i - 1];
    }

    __OSBootDolSimple(0xFFFFFFFF, restart_code, save_start, save_end, 0,
                      argument_count, arguments);
}
