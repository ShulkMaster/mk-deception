#include "dolphin/trk.h"

DSError TRKMessageSend(MessageBuffer* message)
{
    DSError write_error = TRKWriteUARTN(message->data, message->length);
    MWTRACE(1, "MessageSend : cc_write returned %ld\n", write_error);
    return 0;
}
