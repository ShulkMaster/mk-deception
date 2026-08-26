#include "dolphin/trk.h"

extern void TRKTargetAddStopInfo(MessageBuffer* buffer);
extern void TRKTargetAddExceptionInfo(MessageBuffer* buffer);
extern DSError TRKRequestSend(MessageBuffer* buffer, int* request_id, int retries,
                              int timeout, int blocking);

DSError TRKDoNotifyStopped(MessageCommandID command)
{
    int request_id;
    int buffer_id;
    MessageBuffer* message;
    DSError error;
    DSError buffer_error;

    buffer_error = TRKGetFreeBuffer(&buffer_id, &message);
    if ((error = buffer_error) == 0) {
        if (error == 0) {
            if (command == 0x90)
                TRKTargetAddStopInfo(message);
            else
                TRKTargetAddExceptionInfo(message);
        }
        error = TRKRequestSend(message, &request_id, 2, 3, 1);
        if (error == 0)
            TRKReleaseBuffer(request_id);
        TRKReleaseBuffer(buffer_id);
    }
    return error;
}
