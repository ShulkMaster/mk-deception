#include "dolphin/trk.h"
#include "runtime/cstring.h"

struct TRKAccessFileRequest {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u32 handle;
    u16 data_length;
    u8 field_0x0E[0x32];
};

struct TRKOpenFileRequest {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u8 mode;
    u8 field_0x09[3];
    u16 name_length;
    u8 field_0x0E[0x32];
};

struct TRKCloseFileRequest {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u32 handle;
    u8 field_0x0C[0x34];
};

struct TRKPositionFileRequest {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u32 handle;
    u32 position;
    u8 mode;
    u8 field_0x11[0x2F];
};

struct TRKFileSupportReply {
    u32 length;
    u8 command;
    u8 field_0x05[3];
    u32 handle;
    u8 field_0x0C[4];
    u32 io_result;
    u16 data_length;
    u8 field_0x16[2];
    u32 position;
};

enum {
    TRK_READ_FILE_RETRIES = 5,
    TRK_WRITE_FILE_RETRIES = 5,
};

typedef char TRKAccessFileRequestSizeCheck[
    sizeof(struct TRKAccessFileRequest) == 0x40 ? 1 : -1];
typedef char TRKOpenFileRequestSizeCheck[
    sizeof(struct TRKOpenFileRequest) == 0x40 ? 1 : -1];
typedef char TRKCloseFileRequestSizeCheck[
    sizeof(struct TRKCloseFileRequest) == 0x40 ? 1 : -1];
typedef char TRKPositionFileRequestSizeCheck[
    sizeof(struct TRKPositionFileRequest) == 0x40 ? 1 : -1];

DSError TRKSuppAccessFile(u32 file_handle, u8* data, u32* count, DSIOResult* io_result,
                          BOOL need_reply, BOOL read)
{
    DSError error;
    MessageBufferID reply_id;
    MessageBuffer* reply_buffer;
    u32 length;
    MessageBufferID request_id;
    MessageBuffer* request_buffer;
    u32 done;
    u8 reply_io_result;
    u32 reply_length;
    BOOL exit;
    struct TRKAccessFileRequest request;

    if (data == 0 || *count == 0) {
        return 2;
    }

    exit = 0;
    *io_result = 0;
    done = 0;
    error = 0;
    while (!exit && done < *count && error == 0 && *io_result == 0) {
        memset(&request, 0, sizeof(request));

        if (*count - done <= 0x800) {
            length = *count - done;
        } else {
            length = 0x800;
        }

        request.command = read ? TRK_MSG_READ_FILE : TRK_MSG_WRITE_FILE;
        if (read) {
            request.length = sizeof(request);
        } else {
            request.length = length + sizeof(request);
        }
        request.handle = file_handle;
        request.data_length = length;

        TRKGetFreeBuffer(&request_id, &request_buffer);
        error = TRKAppendBuffer_ui8(request_buffer, (u8*)&request, sizeof(request));

        if (!read && error == 0) {
            error = TRKAppendBuffer_ui8(request_buffer, data + done, length);
        }

        if (error == 0) {
            if (need_reply) {
                BOOL poll = read && file_handle == 0;
                struct TRKFileSupportReply* reply;

                error = TRKRequestSend(request_buffer, &reply_id,
                                       read ? TRK_READ_FILE_RETRIES : TRK_WRITE_FILE_RETRIES,
                                       3, !poll);
                if (error == 0) {
                    reply_buffer = TRKGetBuffer(reply_id);
                }

                reply = (struct TRKFileSupportReply*)reply_buffer->data;
                reply_io_result = reply->io_result;
                reply_length = reply->data_length;
                if (read && error == 0 && reply_length <= length) {
                    TRKSetBufferPosition(reply_buffer, sizeof(request));
                    error = TRKReadBuffer_ui8(reply_buffer, data + done, reply_length);
                    if (error == 0x302) {
                        error = 0;
                    }
                }

                if (reply_length != length) {
                    length = reply_length;
                    exit = 1;
                }

                *io_result = reply_io_result;
                TRKReleaseBuffer(reply_id);
            } else {
                error = TRKMessageSend(request_buffer);
            }
        }

        TRKReleaseBuffer(request_id);
        done += length;
    }

    *count = done;
    return error;
}

DSError TRKRequestSend(MessageBuffer* request, MessageBufferID* reply_id,
                       int retries, int timeout, BOOL blocking)
{
    DSError error = 0;
    MessageBuffer* reply_buffer;
    u32 poll_count;
    int attempts;
    u8 reply_command;
    int reply_error;
    BOOL bad_reply = 1;

    *reply_id = -1;

    for (attempts = timeout + 1; attempts != 0 && *reply_id == -1 && error == 0;
         attempts--) {
        MWTRACE(1, "Calling MessageSend\n");
        error = TRKMessageSend(request);
        if (error == 0) {
            if (blocking) {
                poll_count = 0;
            }

            while (1) {
                do {
                    *reply_id = TRKTestForPacket();
                    if (*reply_id != -1) {
                        break;
                    }
                } while (!blocking || ++poll_count < 79999980);

                if (*reply_id == -1) {
                    break;
                }

                bad_reply = 0;

                reply_buffer = TRKGetBuffer(*reply_id);
                TRKSetBufferPosition(reply_buffer, 0);
                OutputData(reply_buffer->data, reply_buffer->length);
                reply_command = reply_buffer->data[4];
                MWTRACE(1, "msg_command : 0x%02x hdr->cmdID 0x%02x\n", reply_command,
                        reply_command);

                if (reply_command >= 0x80) {
                    break;
                }

                TRKProcessInput(*reply_id);
                *reply_id = -1;
            }

            if (*reply_id != -1) {
                if (reply_buffer->length < 0x40) {
                    bad_reply = 1;
                }
                if (error == 0 && !bad_reply) {
                    reply_error = reply_buffer->data[8];
                    MWTRACE(1, "msg_error : 0x%02x\n", reply_error);
                }
                if (error == 0 && !bad_reply) {
                    if ((int)reply_command != 0x80 || reply_error != 0) {
                        MWTRACE(8,
                                "RequestSend : Bad ack or non ack received msg_command : 0x%02x msg_error 0x%02x\n",
                                reply_command, reply_error);
                        bad_reply = 1;
                    }
                }
                if (error != 0 || bad_reply) {
                    TRKReleaseBuffer(*reply_id);
                    *reply_id = -1;
                }
            }
        }
    }

    if (*reply_id == -1) {
        error = 0x800;
    }

    return error;
}

DSError HandleOpenFileSupportRequest(const char* path, u8 mode, u32* handle,
                                     DSIOResult* io_result)
{
    DSError error;
    MessageBufferID reply_id;
    MessageBufferID request_id;
    MessageBuffer* reply_buffer;
    MessageBuffer* request_buffer;
    struct TRKOpenFileRequest request;

    memset(&request, 0, sizeof(request));
    *handle = 0;
    request.command = TRK_MSG_OPEN_FILE;
    request.length = strlen(path) + sizeof(request) + 1;
    request.mode = mode;
    request.name_length = strlen(path) + 1;
    TRKGetFreeBuffer(&request_id, &request_buffer);
    error = TRKAppendBuffer_ui8(request_buffer, (u8*)&request, sizeof(request));

    if (error == 0) {
        error = TRKAppendBuffer_ui8(request_buffer, (const u8*)path, strlen(path) + 1);
    }

    if (error == 0) {
        struct TRKFileSupportReply* reply;

        *io_result = 0;
        error = TRKRequestSend(request_buffer, &reply_id, 7, 3, 0);

        if (error == 0) {
            reply_buffer = TRKGetBuffer(reply_id);
        }

        reply = (struct TRKFileSupportReply*)reply_buffer->data;
        *io_result = reply->io_result;
        *handle = reply->handle;
        TRKReleaseBuffer(reply_id);
    }
    TRKReleaseBuffer(request_id);
    return error;
}

DSError HandleCloseFileSupportRequest(u32 handle, DSIOResult* io_result)
{
    struct TRKCloseFileRequest request;
    MessageBufferID reply_id;
    MessageBufferID request_id;
    DSError error;
    MessageBuffer* request_buffer;
    MessageBuffer* reply_buffer;

    memset(&request, 0, sizeof(request));
    request.command = TRK_MSG_CLOSE_FILE;
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
            struct TRKFileSupportReply* reply = (struct TRKFileSupportReply*)reply_buffer->data;
            *io_result = reply->io_result;
        }
        TRKReleaseBuffer(reply_id);
    }
    TRKReleaseBuffer(request_id);
    return error;
}

DSError HandlePositionFileSupportRequest(u32 handle, u32* position, u8 mode,
                                         DSIOResult* io_result)
{
    DSError error;
    MessageBufferID reply_id;
    MessageBufferID request_id;
    MessageBuffer* request_buffer;
    MessageBuffer* reply_buffer;
    struct TRKPositionFileRequest request;

    memset(&request, 0, sizeof(request));
    request.command = TRK_MSG_POSITION_FILE;
    request.length = sizeof(request);
    request.handle = handle;
    request.position = *position;
    request.mode = mode;
    error = TRKGetFreeBuffer(&request_id, &request_buffer);

    if (error == 0) {
        error = TRKAppendBuffer_ui8(request_buffer, (u8*)&request, sizeof(request));
    }

    if (error == 0) {
        *io_result = 0;
        *position = -1;
        error = TRKRequestSend(request_buffer, &reply_id, 3, 3, 0);

        if (error == 0) {
            reply_buffer = TRKGetBuffer(reply_id);

            if (reply_buffer != 0) {
                struct TRKFileSupportReply* reply = (struct TRKFileSupportReply*)reply_buffer->data;

                *io_result = reply->io_result;
                *position = reply->position;
            }
        }

        TRKReleaseBuffer(reply_id);
    }

    TRKReleaseBuffer(request_id);
    return error;
}
