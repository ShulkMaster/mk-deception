#include "dolphin/trk.h"
#include "runtime/cstring.h"

typedef struct TRKCloseFileRequest {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u32 handle;
    u8 field_0x0C[0x34];
} TRKCloseFileRequest;

typedef struct TRKCloseFileReply {
    u8 field_0x00[0x10];
    u32 io_result;
} TRKCloseFileReply;

typedef char TRKCloseFileRequestSizeCheck[
    sizeof(TRKCloseFileRequest) == 0x40 ? 1 : -1];

extern DSError TRKRequestSend(MessageBuffer* request,
                              MessageBufferID* reply_id, int retries,
                              int timeout, int blocking);

DSError HandleCloseFileSupportRequest(u32 handle, u32* io_result)
{
    TRKCloseFileRequest request;
    MessageBufferID request_id;
    MessageBufferID reply_id;
    MessageBuffer* request_buffer;
    MessageBuffer* reply_buffer;
    DSError error;

    memset(&request, 0, sizeof(request));
    request.command = 0xD3;
    request.length = sizeof(request);
    request.handle = handle;

    error = TRKGetFreeBuffer(&request_id, &request_buffer);
    if (error == 0) {
        error = TRKAppendBuffer_ui8(request_buffer, (u8*)&request,
                                    sizeof(request));
    }
    if (error == 0) {
        *io_result = 0;
        error = TRKRequestSend(request_buffer, &reply_id, 3, 3, 0);
        if (error == 0) {
            reply_buffer = TRKGetBuffer(reply_id);
        }
        if (error == 0) {
            TRKCloseFileReply* reply = (TRKCloseFileReply*)reply_buffer->data;
            *io_result = reply->io_result;
        }
        TRKReleaseBuffer(reply_id);
    }
    TRKReleaseBuffer(request_id);
    return error;
}
