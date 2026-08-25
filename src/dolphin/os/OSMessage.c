#include "dolphin/os.h"

void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* messages, signed long count)
{
    OSInitThreadQueue(&queue->queueSend);
    OSInitThreadQueue(&queue->queueReceive);
    queue->msgArray = messages;
    queue->msgCount = count;
    queue->firstIndex = 0;
    queue->usedCount = 0;
}

int OSSendMessage(OSMessageQueue* queue, OSMessage message, signed long flags)
{
    int enabled;
    signed long lastIndex;

    enabled = OSDisableInterrupts();
    while (queue->msgCount <= queue->usedCount) {
        if (!(flags & 1)) {
            OSRestoreInterrupts(enabled);
            return 0;
        }
        OSSleepThread(&queue->queueSend);
    }

    lastIndex = (queue->firstIndex + queue->usedCount) % queue->msgCount;
    queue->msgArray[lastIndex] = message;
    ++queue->usedCount;
    OSWakeupThread(&queue->queueReceive);
    OSRestoreInterrupts(enabled);
    return 1;
}

int OSReceiveMessage(OSMessageQueue* queue, OSMessage* message, signed long flags)
{
    int enabled = OSDisableInterrupts();

    while (queue->usedCount == 0) {
        if (!(flags & 1)) {
            OSRestoreInterrupts(enabled);
            return 0;
        }
        OSSleepThread(&queue->queueReceive);
    }

    if (message != 0) {
        *message = queue->msgArray[queue->firstIndex];
    }
    queue->firstIndex = (queue->firstIndex + 1) % queue->msgCount;
    --queue->usedCount;
    OSWakeupThread(&queue->queueSend);
    OSRestoreInterrupts(enabled);
    return 1;
}

int OSJamMessage(OSMessageQueue* queue, OSMessage message, signed long flags)
{
    int enabled = OSDisableInterrupts();

    while (queue->msgCount <= queue->usedCount) {
        if (!(flags & 1)) {
            OSRestoreInterrupts(enabled);
            return 0;
        }
        OSSleepThread(&queue->queueSend);
    }

    queue->firstIndex = (queue->firstIndex + queue->msgCount - 1) % queue->msgCount;
    queue->msgArray[queue->firstIndex] = message;
    ++queue->usedCount;
    OSWakeupThread(&queue->queueReceive);
    OSRestoreInterrupts(enabled);
    return 1;
}
