#include "dolphin/trk.h"
#include "dolphin/targimpl.h"

DSError TRKDoNotifyStopped(MessageCommandID command)
{
    int request_id;
    int buffer_id;
    MessageBuffer* message;
    DSError error;

    error = TRKGetFreeBuffer(&buffer_id, &message);
    if (error == 0) {
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
