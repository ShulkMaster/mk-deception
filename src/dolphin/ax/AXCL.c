#include "dolphin/ax.h"
#include "dolphin/ax_internal.h"
#include "dolphin/cache.h"

static unsigned short __AXCommandList[2][384];
static unsigned long __AXCommandListPosition;
static unsigned short* __AXClWrite;
static unsigned long __AXCommandListCycles;
static unsigned long __AXCompressor;
unsigned long __AXClMode;

unsigned long __AXGetCommandListCycles(void)
{
    return __AXCommandListCycles;
}

unsigned long __AXGetCommandListAddress(void)
{
    unsigned long address;

    address = (unsigned long)&__AXCommandList[__AXCommandListPosition][0];
    __AXCommandListPosition += 1;
    __AXCommandListPosition &= 1;
    __AXClWrite = &__AXCommandList[__AXCommandListPosition][0];
    return address;
}

static inline void __AXWriteToCommandList(unsigned short data)
{
    *__AXClWrite = data;
    __AXClWrite++;
}

void __AXNextFrame(void* sbuffer, void* buffer)
{
    unsigned long data;
    unsigned short* command_list;

    __AXCommandListCycles = 0x1A9;
    command_list = __AXClWrite;
    data = __AXGetStudio();
    __AXWriteToCommandList(0);
    __AXWriteToCommandList((unsigned short)(data >> 16));
    __AXWriteToCommandList((unsigned short)data);
    __AXCommandListCycles += 0x2E44;

    switch (__AXClMode) {
    case 0:
        __AXWriteToCommandList(7);
        __AXWriteToCommandList((unsigned short)((unsigned long)sbuffer >> 16));
        __AXWriteToCommandList((unsigned long)sbuffer);
        __AXCommandListCycles += 0x546;
        break;
    case 1:
        __AXWriteToCommandList(0x11);
        __AXWriteToCommandList((unsigned short)((unsigned long)sbuffer >> 16));
        __AXWriteToCommandList((unsigned long)sbuffer);
        __AXCommandListCycles += 0x5E6;
        break;
    case 2:
        break;
    }

    data = (unsigned long)__AXGetPBs();
    __AXWriteToCommandList(2);
    __AXWriteToCommandList((unsigned short)(data >> 16));
    __AXWriteToCommandList((unsigned short)data);
    __AXWriteToCommandList(3);

    if (__AXClMode == 2) {
        __AXGetAuxAInput(&data);
        if (data != 0) {
            __AXWriteToCommandList(0x13);
            __AXWriteToCommandList(data >> 16);
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxAInputDpl2(&data);
            __AXWriteToCommandList(data >> 16);
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxAOutput(&data);
            __AXWriteToCommandList(data >> 16);
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxAOutputDpl2R(&data);
            __AXWriteToCommandList(data >> 16);
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxAOutputDpl2Ls(&data);
            __AXWriteToCommandList(data >> 16);
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxAOutputDpl2Rs(&data);
            __AXWriteToCommandList(data >> 16);
            __AXWriteToCommandList((unsigned short)data);
            __AXCommandListCycles += 0xDED;
        }
        __AXWriteToCommandList(0x10);
        __AXGetAuxBForDPL2(&data);
        __AXWriteToCommandList(data >> 16);
        __AXWriteToCommandList((unsigned short)data);
        __AXGetAuxBOutputDPL2(&data);
        __AXWriteToCommandList(data >> 16);
        __AXWriteToCommandList((unsigned short)data);
        __AXCommandListCycles += 0xDED;
    } else {
        __AXGetAuxAInput(&data);
        if (data != 0) {
            __AXWriteToCommandList(4);
            __AXWriteToCommandList((unsigned short)(data >> 16));
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxAOutput(&data);
            __AXWriteToCommandList((unsigned short)(data >> 16));
            __AXWriteToCommandList((unsigned short)data);
            __AXCommandListCycles += 0xDED;
        }

        __AXGetAuxBInput(&data);
        if (data != 0) {
            __AXWriteToCommandList(5);
            __AXCommandListCycles += 0xDED;
            __AXWriteToCommandList((unsigned short)(data >> 16));
            __AXWriteToCommandList((unsigned short)data);
            __AXGetAuxBOutput(&data);
            __AXWriteToCommandList((unsigned short)(data >> 16));
            __AXWriteToCommandList((unsigned short)data);
        }
    }

    if (__AXCompressor) {
        __AXWriteToCommandList(0x12);
        __AXWriteToCommandList(0x8000);
        __AXWriteToCommandList(0xA);
        __AXWriteToCommandList((unsigned long)__AXCompressorTable >> 16);
        __AXWriteToCommandList((unsigned long)__AXCompressorTable);
        __AXCommandListCycles += 0xBB8;
    }

    __AXWriteToCommandList(0xE);
    __AXWriteToCommandList((unsigned short)((unsigned long)sbuffer >> 16));
    __AXWriteToCommandList((unsigned long)sbuffer);
    __AXWriteToCommandList((unsigned short)((unsigned long)buffer >> 16));
    __AXWriteToCommandList((unsigned long)buffer);
    __AXCommandListCycles += 0x2710;
    __AXWriteToCommandList(0xF);
    __AXCommandListCycles += 2;
    DCFlushRange(command_list, 0x300);
}

void __AXClInit(void)
{
    __AXClMode = 0;
    __AXCommandListPosition = 0;
    __AXClWrite = (void*)&__AXCommandList;
    __AXCompressor = 1;
}

void AXSetMode(unsigned long mode)
{
    if (__AXClMode != mode) {
        __AXClMode = mode;
    }
}
