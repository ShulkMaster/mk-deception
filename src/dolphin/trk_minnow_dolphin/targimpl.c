#include "dolphin/targimpl.h"
#include "runtime/cstring.h"
#include "runtime/asm_sequences.inc"

#define INSTR_NOP 0x60000000
#define INSTR_BLR 0x4E800020
#define INSTR_PSQ_ST(psr, offset, base, w, gqr) \
    (0xF0000000 | ((psr) << 21) | ((base) << 16) | ((w) << 15) | ((gqr) << 12) | (offset))
#define INSTR_PSQ_L(psr, offset, base, w, gqr) \
    (0xE0000000 | ((psr) << 21) | ((base) << 16) | ((w) << 15) | ((gqr) << 12) | (offset))
#define INSTR_STW(source, offset, base) \
    (0x90000000 | ((source) << 21) | ((base) << 16) | (offset))
#define INSTR_LWZ(destination, offset, base) \
    (0x80000000 | ((destination) << 21) | ((base) << 16) | (offset))
#define INSTR_STFD(source, offset, base) \
    (0xD8000000 | ((source) << 21) | ((base) << 16) | (offset))
#define INSTR_LFD(destination, offset, base) \
    (0xC8000000 | ((destination) << 21) | ((base) << 16) | (offset))
#define INSTR_MFSPR(destination, spr) \
    (0x7C000000 | ((destination) << 21) | (((spr) & 0xFE0) << 6) | (((spr) & 0x1F) << 16) | 0x2A6)
#define INSTR_MTSPR(spr, source) \
    (0x7C000000 | ((source) << 21) | (((spr) & 0xFE0) << 6) | (((spr) & 0x1F) << 16) | 0x3A6)

enum {
    TRK_SPR_GQR0 = 912,
    TRK_SPR_HID2 = 920,
    TRK_SPR_FPECR = 1022,
};

struct TRKStopInfoPacket {
    u32 length;
    u8 command;
    u8 reserved_05[3];
    u32 pc;
    u32 instruction;
    u32 exception_id;
    u8 reserved_14[0x2C];
};

typedef char TRKStopInfoPacketSizeCheck[
    sizeof(struct TRKStopInfoPacket) == 0x40 ? 1 : -1];

typedef struct TRKMemoryRange {
    u8* start;
    u8* end;
    BOOL readable;
    BOOL writeable;
} TRKMemoryRange;

typedef struct TRKRestoreFlags {
    u8 tbr;
    u8 dec;
    u8 linker_padding[7];
} TRKRestoreFlags;

typedef struct TRKStepStatus {
    BOOL active;
    int type;
    u32 count;
    u32 range_start;
    u32 range_end;
} TRKStepStatus;

typedef void (*TRKRegisterAccessFunction)(void* value, void* scratch);

u32 __TRK_get_MSR(void);
void __TRK_set_MSR(u32 msr);
static void TRK_ppc_memcpy(void* destination, const void* source, int length,
                           u32 destination_msr, u32 source_msr);
void TRKInterruptHandler(void);
void TRKExceptionHandler(u16 exception_id);
void TRKInterruptHandlerEnableInterrupts(void);
void TRKUARTInterruptHandler(void);
void TRKSaveExtended1Block(void);
void TRKRestoreExtended1Block(void);
void ReadFPSCR(void* value);
void WriteFPSCR(void* value);
DSError TRKPPCAccessSPR(void* value, u32 spr, BOOL read);
DSError TRKPPCAccessPairedSingleRegister(void* value, u32 psr, BOOL read);
DSError TRKPPCAccessFPRegister(void* value, u32 fpr, BOOL read);
DSError TRKPPCAccessSpecialReg(void* value, u32* access_function, BOOL read);
static BOOL TRKTargetCheckStep(void);

const TRKMemoryRange gTRKMemMap[1] = { { (u8*)0, (u8*)-1, 1, 1 } };

TRKRestoreFlags gTRKRestoreFlags = { 0, 0 };
static TRKExceptionStatus gTRKExceptionStatus = { { 0, 0, 0 }, 1, 0 };
static TRKStepStatus gTRKStepStatus = { 0, 0, 0, 0 };

static u16 TRK_saved_exceptionID = 0;
TRKTargetState gTRKState;
TRKDefaultRegisters gTRKSaveState;
TRKCPUState gTRKCPUState;
u32 TRKvalue128_temp[4];

DSError TRKValidMemory32(const void* address, u32 length, int access)
{
    DSError error = 0x700;
    const u8* start;
    const u8* end;
    s32 index;

    start = (const u8*)address;
    end = (const u8*)address + (length - 1);

    if (end < start) {
        return 0x700;
    }

    for (index = 0; index < (s32)(sizeof(gTRKMemMap) / sizeof(gTRKMemMap[0])); index++) {
        if (start <= gTRKMemMap[index].end && end >= gTRKMemMap[index].start) {
            if ((access == 0 && !gTRKMemMap[index].readable) ||
                (access == 1 && !gTRKMemMap[index].writeable)) {
                error = 0x700;
            } else {
                error = 0;

                if (start < gTRKMemMap[index].start) {
                    error = TRKValidMemory32(start, (u32)(gTRKMemMap[index].start - start),
                                             access);
                }

                if (error == 0 && end > gTRKMemMap[index].end) {
                    error = TRKValidMemory32(gTRKMemMap[index].end,
                                             (u32)(end - gTRKMemMap[index].end), access);
                }
            }

            break;
        }
    }

    return error;
}

DSError TRKTargetAccessMemory(void* data, u32 start, u32* length,
                              int access_options, BOOL read)
{
    DSError error;
    u32 msr;
    void* address;
    u32 target_msr;
    TRKExceptionStatus saved_status = gTRKExceptionStatus;

    gTRKExceptionStatus.exception_detected = 0;

    address = (void*)TRKTargetTranslate(start);
    error = TRKValidMemory32(address, *length, read == 0);

    if (error != 0) {
        *length = 0;
    } else {
        msr = __TRK_get_MSR();
        target_msr = msr | gTRKCPUState.extended1.msr & 0x10;

        if (read) {
            TRK_ppc_memcpy(data, address, *length, msr, target_msr);
        } else {
            TRK_ppc_memcpy(address, data, *length, target_msr, msr);
            TRK_flush_cache((u32)address, *length);
            if ((void*)start != address) {
                TRK_flush_cache(start, *length);
            }
        }
    }

    if (gTRKExceptionStatus.exception_detected) {
        *length = 0;
        error = 0x702;
    }

    gTRKExceptionStatus = saved_status;
    return error;
}

DSError TRKTargetReadInstruction(u32* instruction, u32 address)
{
    u32 length = sizeof(*instruction);
    DSError error;

    error = TRKTargetAccessMemory(instruction, address, &length, 0, 1);
    if (error == 0 && length != sizeof(*instruction)) {
        error = 0x700;
    }
    return error;
}

DSError TRKTargetAccessDefault(u32 first_register, u32 last_register,
                               MessageBuffer* message, u32* registers_length,
                               BOOL read)
{
    DSError error;
    u32 count;
    u32* data;
    TRKExceptionStatus saved_status;

    if (last_register > 0x24) {
        return 0x701;
    }

    saved_status = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_detected = 0;

    data = gTRKCPUState.default_registers.gpr + first_register;
    count = (last_register - first_register) + 1;
    *registers_length = count * sizeof(u32);

    if (read) {
        error = TRKAppendBuffer_ui32(message, data, count);
    } else {
        error = TRKReadBuffer_ui32(message, data, count);
    }

    if (gTRKExceptionStatus.exception_detected) {
        *registers_length = 0;
        error = 0x702;
    }

    gTRKExceptionStatus = saved_status;
    return error;
}

DSError TRKTargetAccessFP(u32 first_register, u32 last_register,
                          MessageBuffer* message, u32* registers_length,
                          BOOL read)
{
    u64 value;
    DSError error;
    TRKExceptionStatus saved_status;
    u32 current;

    if (last_register > 0x21) {
        return 0x701;
    }

    saved_status = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_detected = 0;

    __TRK_set_MSR(__TRK_get_MSR() | 0x2000);

    *registers_length = 0;
    error = 0;

    for (current = first_register; current <= last_register && error == 0;
         current++, *registers_length += sizeof(f64)) {
        if (read) {
            TRKPPCAccessFPRegister(&value, current, read);
            error = TRKAppendBuffer1_ui64(message, value);
        } else {
            TRKReadBuffer1_ui64(message, &value);
            error = TRKPPCAccessFPRegister(&value, current, read);
        }
    }

    if (gTRKExceptionStatus.exception_detected) {
        *registers_length = 0;
        error = 0x702;
    }

    gTRKExceptionStatus = saved_status;
    return error;
}

DSError TRKTargetAccessExtended1(u32 first_register, u32 last_register,
                                 MessageBuffer* message, u32* registers_length,
                                 BOOL read)
{
    TRKExceptionStatus saved_status;
    int error;
    u32* data;
    int count;

    if (last_register > 0x60) {
        return 0x701;
    }

    saved_status = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_detected = 0;

    *registers_length = 0;

    if (first_register <= last_register) {
        data = (u32*)&gTRKCPUState.extended1 + first_register;
        count = last_register - first_register + 1;
        *registers_length += count * sizeof(u32);

        if (read) {
            error = TRKAppendBuffer_ui32(message, data, count);
        } else {
            if (data <= &gTRKCPUState.extended1.tbu &&
                data + count - 1 >= &gTRKCPUState.extended1.tbl) {
                gTRKRestoreFlags.tbr = 1;
            }

            if (data <= &gTRKCPUState.extended1.dec &&
                data + count - 1 >= &gTRKCPUState.extended1.dec) {
                gTRKRestoreFlags.dec = 1;
            }
            error = TRKReadBuffer_ui32(message, data, count);
        }
    }

    if (gTRKExceptionStatus.exception_detected) {
        *registers_length = 0;
        error = 0x702;
    }

    gTRKExceptionStatus = saved_status;
    return error;
}

DSError TRKTargetAccessExtended2(u32 first_register, u32 last_register,
                                 MessageBuffer* message, u32* registers_length,
                                 BOOL read)
{
    TRKExceptionStatus saved_status;
    u32 index;
    u32 spr_value[1];
    u32 value[2];
    DSError error;

    if (last_register > 0x1F) {
        return 0x701;
    }

    saved_status = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_detected = 0;

    TRKPPCAccessSPR(spr_value, TRK_SPR_HID2, 1);

    spr_value[0] |= 0xA0000000;
    TRKPPCAccessSPR(spr_value, TRK_SPR_HID2, 0);

    spr_value[0] = 0;
    TRKPPCAccessSPR(spr_value, TRK_SPR_GQR0, 0);

    *registers_length = 0;
    error = 0;

    for (index = first_register; index <= last_register && error == 0; index++) {
        if (read) {
            error = TRKPPCAccessPairedSingleRegister((u64*)value, index, read);
            error = TRKAppendBuffer1_ui64(message, *(u64*)value);
        } else {
            error = TRKReadBuffer1_ui64(message, (u64*)value);
            error = TRKPPCAccessPairedSingleRegister((u64*)value, index, read);
        }

        *registers_length += sizeof(u64);
    }

    if (gTRKExceptionStatus.exception_detected) {
        *registers_length = 0;
        error = 0x702;
    }

    gTRKExceptionStatus = saved_status;
    return error;
}

void TRKPostInterruptEvent(void)
{
    int event_type;
    u32 instruction;
    TRKEvent event;

    if (gTRKState.input_activated) {
        gTRKState.input_activated = 0;
    } else {
        switch (gTRKCPUState.extended1.exception_id & 0xFFFF) {
        case 0xD00:
        case 0x700:
            TRKTargetReadInstruction(&instruction, gTRKCPUState.default_registers.pc);

            if (instruction == 0x0FE00000) {
                event_type = 5;
            } else {
                event_type = 3;
            }
            break;
        default:
            event_type = 4;
            break;
        }

        TRKConstructEvent(&event, event_type);
        TRKPostEvent(&event);
    }
}

DSError TRKTargetInterrupt(TRKEvent* event)
{
    DSError error = 0;

    switch (event->event_type) {
    case 3:
    case 4:
        if (TRKTargetCheckStep() == 0) {
            TRKTargetSetStopped(1);
            error = TRKDoNotifyStopped(0x90);
        }
        break;
    default:
        break;
    }

    return error;
}

void TRKTargetAddStopInfo(MessageBuffer* message)
{
    struct TRKStopInfoPacket packet;
    u32 instruction;

    memset(&packet, 0, sizeof(packet));
    packet.length = sizeof(packet);
    packet.command = 0x90;
    packet.pc = gTRKCPUState.default_registers.pc;
    TRKTargetReadInstruction(&instruction, gTRKCPUState.default_registers.pc);
    packet.instruction = instruction;
    packet.exception_id = (u16)gTRKCPUState.extended1.exception_id;
    TRKAppendBuffer_ui8(message, (u8*)&packet, sizeof(packet));
}

void TRKTargetAddExceptionInfo(MessageBuffer* message)
{
    struct TRKStopInfoPacket packet;
    u32 instruction;

    memset(&packet, 0, sizeof(packet));
    packet.length = sizeof(packet);
    packet.command = 0x91;
    packet.pc = gTRKExceptionStatus.exception_info.pc;
    TRKTargetReadInstruction(&instruction, gTRKExceptionStatus.exception_info.pc);
    packet.instruction = instruction;
    packet.exception_id = gTRKExceptionStatus.exception_info.exception_id;
    TRKAppendBuffer_ui8(message, (u8*)&packet, sizeof(packet));
}

DSError TRKTargetEnableTrace(BOOL enable)
{
    if (enable) {
        gTRKCPUState.extended1.msr = gTRKCPUState.extended1.msr | 0x400;
    } else {
        gTRKCPUState.extended1.msr = gTRKCPUState.extended1.msr & ~0x400;
    }
    return 0;
}

BOOL TRKTargetStepDone(void)
{
    BOOL done = 1;

    if (gTRKStepStatus.active && (u16)gTRKCPUState.extended1.exception_id == 0xD00) {
        switch (gTRKStepStatus.type) {
        case 0:
            if (gTRKStepStatus.count > 0) {
                done = 0;
            }
            break;
        case 1:
            if (gTRKCPUState.default_registers.pc >= gTRKStepStatus.range_start &&
                gTRKCPUState.default_registers.pc <= gTRKStepStatus.range_end) {
                done = 0;
            }
            break;
        default:
            break;
        }
    }

    return done;
}

DSError TRKTargetDoStep(void)
{
    gTRKStepStatus.active = 1;
    MWTRACE(1, "TargetDoStep()\n");
    TRKTargetEnableTrace(1);

    if (gTRKStepStatus.type == 0 || gTRKStepStatus.type == 0x10) {
        gTRKStepStatus.count--;
    }

    TRKTargetSetStopped(0);
    return 0;
}

static BOOL TRKTargetCheckStep(void)
{
    if (gTRKStepStatus.active) {
        TRKTargetEnableTrace(0);

        if (TRKTargetStepDone()) {
            gTRKStepStatus.active = 0;
        } else {
            TRKTargetDoStep();
        }
    }

    return gTRKStepStatus.active;
}

DSError TRKTargetSingleStep(u32 count, BOOL step_over)
{
    DSError error = 0;

    if (step_over) {
        error = 0x703;
    } else {
        gTRKStepStatus.count = count;
        gTRKStepStatus.type = 0;
        error = TRKTargetDoStep();
    }

    return error;
}

DSError TRKTargetStepOutOfRange(u32 range_start, u32 range_end, BOOL step_over)
{
    DSError error = 0;

    if (step_over) {
        error = 0x703;
    } else {
        gTRKStepStatus.type = 1;
        gTRKStepStatus.range_start = range_start;
        gTRKStepStatus.range_end = range_end;
        error = TRKTargetDoStep();
    }

    return error;
}

u32 TRKTargetGetPC(void)
{
    return gTRKCPUState.default_registers.pc;
}

DSError TRKTargetSupportRequest(void)
{
    DSIOResult io_result;
    u32* length;
    MessageCommandID command;
    DSError error;
    u32 position;
    TRKEvent event;

    command = gTRKCPUState.default_registers.gpr[3];
    if (command != TRK_MSG_READ_FILE && command != TRK_MSG_WRITE_FILE &&
        command != TRK_MSG_OPEN_FILE && command != TRK_MSG_CLOSE_FILE &&
        command != TRK_MSG_POSITION_FILE) {
        TRKConstructEvent(&event, 4);
        TRKPostEvent(&event);
        return 0;
    } else if (command == TRK_MSG_OPEN_FILE) {
        error = HandleOpenFileSupportRequest(
            (const char*)gTRKCPUState.default_registers.gpr[4],
            (u8)gTRKCPUState.default_registers.gpr[5],
            (u32*)gTRKCPUState.default_registers.gpr[6], &io_result);

        if (io_result == 0 && error != 0) {
            io_result = 1;
        }

        gTRKCPUState.default_registers.gpr[3] = io_result;
    } else if (command == TRK_MSG_CLOSE_FILE) {
        error = HandleCloseFileSupportRequest(gTRKCPUState.default_registers.gpr[4],
                                              &io_result);

        if (io_result == 0 && error != 0) {
            io_result = 1;
        }

        gTRKCPUState.default_registers.gpr[3] = io_result;
    } else if (command == TRK_MSG_POSITION_FILE) {
        position = *(u32*)gTRKCPUState.default_registers.gpr[5];
        error = HandlePositionFileSupportRequest(
            gTRKCPUState.default_registers.gpr[4], &position,
            (u8)gTRKCPUState.default_registers.gpr[6], &io_result);

        if (io_result == 0 && error != 0) {
            io_result = 1;
        }

        gTRKCPUState.default_registers.gpr[3] = io_result;
        *(u32*)gTRKCPUState.default_registers.gpr[5] = position;
    } else {
        length = (u32*)gTRKCPUState.default_registers.gpr[5];
        error = TRKSuppAccessFile(gTRKCPUState.default_registers.gpr[4],
                                  (u8*)gTRKCPUState.default_registers.gpr[6], length,
                                  &io_result, 1, command == TRK_MSG_READ_FILE);

        if (io_result == 0 && error != 0) {
            io_result = 1;
        }

        gTRKCPUState.default_registers.gpr[3] = io_result;

        if (command == TRK_MSG_READ_FILE) {
            TRK_flush_cache(gTRKCPUState.default_registers.gpr[6], *length);
        }
    }

    gTRKCPUState.default_registers.pc += 4;
    return error;
}

BOOL TRKTargetStopped(void)
{
    return gTRKState.stopped;
}

void TRKTargetSetStopped(BOOL stopped)
{
    gTRKState.stopped = stopped;
}

DSError TRKTargetStop(void)
{
    gTRKState.stopped = 1;
    return 0;
}

DSError TRKPPCAccessSPR(void* value, u32 spr, BOOL read)
{
    u32 access_function[10] = { INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP,
                                INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP };

    if (read) {
        access_function[0] = INSTR_MFSPR(4, spr);
        access_function[1] = INSTR_STW(4, 0, 3);
    } else {
        access_function[0] = INSTR_LWZ(4, 0, 3);
        access_function[1] = INSTR_MTSPR(spr, 4);
    }

    return TRKPPCAccessSpecialReg(value, access_function, read);
}

DSError TRKPPCAccessPairedSingleRegister(void* value, u32 psr, BOOL read)
{
    u32 access_function[10] = { INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP,
                                INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP };

    if (read) {
        access_function[0] = INSTR_PSQ_ST(psr, 0, 3, 0, 0);
    } else {
        access_function[0] = INSTR_PSQ_L(psr, 0, 3, 0, 0);
    }

    return TRKPPCAccessSpecialReg(value, access_function, read);
}

DSError TRKPPCAccessFPRegister(void* value, u32 fpr, BOOL read)
{
    DSError error = 0;
    u32 access_function[10] = { INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP,
                                INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP };

    if (fpr < 0x20) {
        if (read) {
            access_function[0] = INSTR_STFD(fpr, 0, 3);
        } else {
            access_function[0] = INSTR_LFD(fpr, 0, 3);
        }

        error = TRKPPCAccessSpecialReg(value, access_function, read);
    } else if (fpr == 0x20) {
        if (read) {
            ReadFPSCR(value);
        } else {
            WriteFPSCR(value);
        }

        *(u64*)value &= 0xFFFFFFFF;
    } else if (fpr == 0x21) {
        if (!read) {
            *(u32*)value = *((u32*)value + 1);
        }

        error = TRKPPCAccessSPR(value, TRK_SPR_FPECR, read);
        if (read) {
            *(u64*)value = *(u32*)value & 0xFFFFFFFFLL;
        }
    }

    return error;
}

DSError TRKPPCAccessSpecialReg(void* value, u32* access_function, BOOL read)
{
    TRKRegisterAccessFunction access;

    access_function[9] = INSTR_BLR;
    access = (TRKRegisterAccessFunction)access_function;

    TRK_flush_cache((u32)access_function, 10 * sizeof(u32));
    access(value, TRKvalue128_temp);

    return 0;
}

void TRKTargetSetInputPendingPtr(volatile u8* input_pending)
{
    gTRKState.input_pending = input_pending;
}

DSError TRKTargetAccessARAM(void* data, u32 start, u32* length, BOOL read)
{
    DSError error;
    TRKExceptionStatus saved_status;

    error = 0;
    saved_status = gTRKExceptionStatus;

    gTRKExceptionStatus.exception_detected = 0;

    if (read) {
        TRK__read_aram(data, start, length);
    } else {
        TRK__write_aram(data, start, length);
    }

    if (gTRKExceptionStatus.exception_detected) {
        *length = 0;
        error = 0x702;
    }

    gTRKExceptionStatus = saved_status;
    return error;
}

asm u32 __TRK_get_MSR(void)
{
    SEQ___TRK_get_MSR();
}

asm void __TRK_set_MSR(u32 msr)
{
    SEQ___TRK_set_MSR();
}

static asm void TRK_ppc_memcpy(void* destination, const void* source, int length,
                               u32 destination_msr, u32 source_msr)
{
    SEQ_TRK_ppc_memcpy();
}

asm void TRKInterruptHandler(void)
{
    SEQ_TRKInterruptHandler();
}

asm void TRKExceptionHandler(u16 exception_id)
{
    SEQ_TRKExceptionHandler();
}

asm void TRKSwapAndGo(void)
{
    SEQ_TRKSwapAndGo();
}

asm void TRKInterruptHandlerEnableInterrupts(void)
{
    SEQ_TRKInterruptHandlerEnableInterrupts();
}

asm void ReadFPSCR(void* value)
{
    SEQ_ReadFPSCR();
}

asm void WriteFPSCR(void* value)
{
    SEQ_WriteFPSCR();
}
