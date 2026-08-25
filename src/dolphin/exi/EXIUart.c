#include "dolphin/exi.h"
#include "dolphin/os.h"

static signed long Chan;
static unsigned long Dev;
static unsigned long Enabled;
static unsigned long BarnacleEnabled;

static int ProbeBarnacle(signed long chan, unsigned long dev,
                         unsigned long* revision)
{
    unsigned long command;
    int error;

    if (chan != 2 && dev == 0 && !EXIAttach(chan, 0)) {
        return 0;
    }

    error = !EXILock(chan, dev, 0);
    if (!error) {
        error = !EXISelect(chan, dev, EXI_FREQ_1M);
        if (!error) {
            command = 0x20011300;
            error = 0;
            error |= !EXIImm(chan, &command, sizeof(command), EXI_WRITE, 0);
            error |= !EXISync(chan);
            error |= !EXIImm(chan, revision, sizeof(*revision), EXI_READ, 0);
            error |= !EXISync(chan);
            error |= !EXIDeselect(chan);
        }
        EXIUnlock(chan);
    }

    if (chan != 2 && dev == 0) {
        EXIDetach(chan);
    }

    if (error) {
        return 0;
    }
    if (*revision != 0xFFFFFFFF) {
        return 1;
    }
    return 0;
}

void __OSEnableBarnacle(signed long chan, unsigned long dev)
{
    unsigned long id;

    if (!EXIGetID(chan, dev, &id)) {
        return;
    }

    switch (id) {
    case EXI_MEMORY_CARD_59:
    case EXI_MEMORY_CARD_123:
    case EXI_MEMORY_CARD_251:
    case EXI_MEMORY_CARD_507:
    case EXI_USB_ADAPTER:
    case EXI_NPDP_GDEV:
    case EXI_MODEM:
    case 0x03010000:
    case 0x04020100:
    case EXI_ETHER:
    case 0x04020300:
    case 0x04220000:
    case EXI_RS232C:
    case EXI_MIC:
    case EXI_AD16:
    case EXI_STREAM_HANGER:
    case 0x80000004:
    case 0x80000008:
    case 0x80000010:
    case 0x80000020:
    case 0xFFFFFFFF:
        break;
    default:
        if (ProbeBarnacle(chan, dev, &id)) {
            Chan = chan;
            Dev = dev;
            Enabled = BarnacleEnabled = 0xA5FF005A;
        }
        break;
    }
}

int InitializeUART(void)
{
    if (BarnacleEnabled == 0xA5FF005A) {
        return 0;
    }

    if (!(OSGetConsoleType() & 0x10000000)) {
        Enabled = 0;
        return 2;
    }

    Chan = 0;
    Dev = 1;
    Enabled = 0xA5FF005A;
    return 0;
}

static int QueueLength(void)
{
    unsigned long command;

    if (!EXISelect(Chan, Dev, EXI_FREQ_8M)) {
        return -1;
    }

    command = 0x20010000;
    EXIImm(Chan, &command, sizeof(command), EXI_WRITE, 0);
    EXISync(Chan);
    EXIImm(Chan, &command, 1, EXI_READ, 0);
    EXISync(Chan);
    EXIDeselect(Chan);
    return 16 - (command >> 24);
}

int WriteUARTN(void* buffer, unsigned long length)
{
    unsigned long command;
    signed long transfer_length;
    signed long queue_length;
    char* pointer;
    int locked;
    int error;

    if (Enabled - 0xA5FF0000 != 0x5A) {
        return 2;
    }

    locked = EXILock(Chan, Dev, 0);
    if (locked == 0) {
        return 0;
    } else {
        pointer = buffer;
    }

    while ((unsigned long)pointer - (unsigned long)buffer < length) {
        if (*(signed char*)pointer == '\n') {
            *pointer = '\r';
        }
        pointer++;
    }

    error = 0;
    command = 0xA0010000;
    while (length != 0) {
        queue_length = QueueLength();
        if (queue_length < 0) {
            error = 3;
            break;
        }

        if (queue_length >= 12 || (unsigned long)queue_length >= length) {
            if (!EXISelect(Chan, Dev, EXI_FREQ_8M)) {
                error = 3;
                break;
            }

            EXIImm(Chan, &command, sizeof(command), EXI_WRITE, 0);
            EXISync(Chan);
            while (queue_length != 0 && length != 0) {
                if (queue_length < 4 && (unsigned long)queue_length < length) {
                    break;
                }

                transfer_length = length < 4 ? length : 4;
                EXIImm(Chan, buffer, transfer_length, EXI_WRITE, 0);
                buffer = (char*)buffer + transfer_length;
                length -= transfer_length;
                queue_length -= transfer_length;
                EXISync(Chan);
            }
            EXIDeselect(Chan);
        }
    }

    EXIUnlock(Chan);
    return error;
}
