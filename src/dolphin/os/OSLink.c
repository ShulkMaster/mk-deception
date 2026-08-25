typedef struct OSModuleQueue {
    void* head;
    void* tail;
} OSModuleQueue;

extern OSModuleQueue __OSModuleInfoList : 0x800030C8;
extern const void* __OSStringTable : 0x800030D0;

void __OSModuleInit(void)
{
    __OSModuleInfoList.head = __OSModuleInfoList.tail = 0;
    __OSStringTable = 0;
}
