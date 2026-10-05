#include "dolphin/os.h"

struct OSModuleInfo;

struct OSModuleQueue {
    struct OSModuleInfo* head;
    struct OSModuleInfo* tail;
};

extern struct OSModuleQueue __OSModuleInfoList : 0x800030C8;
extern const void* __OSStringTable : 0x800030D0;

void __OSModuleInit(void)
{
    __OSModuleInfoList.head = __OSModuleInfoList.tail = 0;
    __OSStringTable = 0;
}
